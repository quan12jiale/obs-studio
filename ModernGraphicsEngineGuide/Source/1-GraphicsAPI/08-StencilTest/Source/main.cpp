#include <QApplication>
#include <QTime>
//#include "QEngineApplication.h"
#include "Render/RHI/QRhiWindow.h"

static QVector2D Vertices[3] = {
	{ 0.0f,    0.5f},
	{-0.5f,   -0.5f},
	{ 0.5f,   -0.5f},
};

struct UniformBlock {
	QGenericMatrix<4, 4, float> MVP;
};

class TriangleWindow : public QRhiWindow {
private:
	QRhiSignal mSigInit;
	QRhiSignal mSigSubmit;

	QScopedPointer<QRhiBuffer> mMaskVertexBuffer;
	QScopedPointer<QRhiGraphicsPipeline> mMaskPipeline;

	QScopedPointer<QRhiBuffer> mVertexBuffer;
	QScopedPointer<QRhiShaderResourceBindings> mShaderBindings;
	QScopedPointer<QRhiGraphicsPipeline> mPipeline;
public:
	TriangleWindow(QRhiHelper::InitParams inInitParams)
		: QRhiWindow(inInitParams) {
		mSigInit.request();
		mSigSubmit.request();
	}
protected:
	virtual void onRenderTick() override {
		if (mSigInit.ensure()) {	
			mMaskVertexBuffer.reset(mRhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(Vertices)));
			mMaskVertexBuffer->create();

			mVertexBuffer.reset(mRhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(Vertices)));
			mVertexBuffer->create();

			QRhiVertexInputLayout inputLayout;
			inputLayout.setBindings({
				QRhiVertexInputBinding(sizeof(QVector2D))
			});

			inputLayout.setAttributes({
				QRhiVertexInputAttribute(0, 0 , QRhiVertexInputAttribute::Float2, 0),
			});

			QShader vs = QRhiHelper::newShaderFromCode(QShader::VertexStage, R"(#version 440
				layout(location = 0) in vec2 position;		
				out gl_PerVertex { 
					vec4 gl_Position;
				};
				void main(){
					gl_Position = vec4(position,0.0f,1.0f);
				}
			)");
			Q_ASSERT(vs.isValid());

			QShader fs = QRhiHelper::newShaderFromCode(QShader::FragmentStage, R"(#version 440	
				layout (location = 0) out vec4 fragColor;	
				void main(){
					fragColor = vec4(1);
				}
			)");
			Q_ASSERT(fs.isValid());

			mShaderBindings.reset(mRhi->newShaderResourceBindings());
			mShaderBindings->create();

			mMaskPipeline.reset(mRhi->newGraphicsPipeline());
			mMaskPipeline->setVertexInputLayout(inputLayout);
			mMaskPipeline->setShaderResourceBindings(mShaderBindings.get());
			mMaskPipeline->setSampleCount(mSwapChain->sampleCount());
			mMaskPipeline->setRenderPassDescriptor(mSwapChainPassDesc.get());
			mMaskPipeline->setShaderStages({
				QRhiShaderStage(QRhiShaderStage::Vertex, vs),
				QRhiShaderStage(QRhiShaderStage::Fragment, fs)
			});
			mMaskPipeline->setFlags(QRhiGraphicsPipeline::Flag::UsesStencilRef);			//¿ªÆôÁ÷Ë®ÏßµÄÄ£°æ²âÊÔ¹¦ÄÜ
			mMaskPipeline->setStencilTest(true);											//¿ªÆôÄ£°æ²âÊÔ
			QRhiGraphicsPipeline::StencilOpState maskStencilState;
			maskStencilState.compareOp = QRhiGraphicsPipeline::CompareOp::Never;			//¸Ă¹ÜÏßÓĂÓÚ»æÖÆÄ£°æ£¨ƠÚƠÖ£©£¬Î̉ĂÇ²»Ï£ÍûËüÔÚRTÉÏ»æÖÆÈÎºÎÆ¬¶ÎÑƠÉ«£¬̣̉´ËÈĂËüµÄÆ¬¶ÎÓÀÔ¶²»»áÍ¨¹ưÄ£°æ²âÊÔ
			maskStencilState.failOp = QRhiGraphicsPipeline::StencilOp::Replace;				//ÉèÖĂµ±¸ĂÆ¬¶ÎĂ»ÓĐÍ¨¹ưÄ£°æ²âÊÔÊ±£¬Ê¹ÓĂStencilRef̀î³äÄ£°æ»º³åÇø
			mMaskPipeline->setStencilFront(maskStencilState);								//Ö¸¶¨ƠưĂæµÄÄ£°æ²âÊÔ
			mMaskPipeline->setStencilBack(maskStencilState);								//Ö¸¶¨±³ĂæµÄÄ£°æ²âÊÔ
			mMaskPipeline->create();

			mPipeline.reset(mRhi->newGraphicsPipeline());
			mPipeline->setVertexInputLayout(inputLayout);
			mPipeline->setShaderResourceBindings(mShaderBindings.get());
			mPipeline->setSampleCount(mSwapChain->sampleCount());
			mPipeline->setRenderPassDescriptor(mSwapChainPassDesc.get());
			mPipeline->setShaderStages({
				QRhiShaderStage(QRhiShaderStage::Vertex, vs),
				QRhiShaderStage(QRhiShaderStage::Fragment, fs)
			});
			mPipeline->setFlags(QRhiGraphicsPipeline::Flag::UsesStencilRef);				//¿ªÆôÁ÷Ë®ÏßµÄÄ£°æ²âÊÔ¹¦ÄÜ
			mPipeline->setStencilTest(true);												//¿ªÆôÄ£°æ²âÊÔ
			QRhiGraphicsPipeline::StencilOpState stencilState;
			stencilState.compareOp = QRhiGraphicsPipeline::CompareOp::Equal;				//Î̉ĂÇÏ£ÍûÔÚµ±Ç°¹ÜÏßµÄStencilRefµÈÓÚÄ£°æ»º³åÇøÉÏµÄÆ¬¶ÎÖµÊ±²ÅÍ¨¹ưÄ£°æ²âÊÔ
			stencilState.passOp = QRhiGraphicsPipeline::StencilOp::Keep;					//ÔÚÍ¨¹ư²âÊÔºó²»»á¶ÔÄ£°æ»º³åÇø½øĐĐ¸³Öµ
			stencilState.failOp = QRhiGraphicsPipeline::StencilOp::Keep;					//ÔÚĂ»ÓĐÍ¨¹ư²âÊÔÊ±̉²²»»á¶ÔÄ£°æ»º³åÇø½øĐĐ¸³Öµ
			mPipeline->setStencilFront(stencilState);
			mPipeline->setStencilBack(stencilState);
			mPipeline->create();

		}

		QRhiRenderTarget* renderTarget = mSwapChain->currentFrameRenderTarget();
		QRhiCommandBuffer* cmdBuffer = mSwapChain->currentFrameCommandBuffer();

		if (mSigSubmit.ensure()) {
			QRhiResourceUpdateBatch* batch = mRhi->nextResourceUpdateBatch();
			batch->uploadStaticBuffer(mMaskVertexBuffer.get(), Vertices);					//ÉÏ´«Ä£°æ£¨ƠÚƠÖ£©µÄ¶¥µăÊư¾Ư£¬ËüÊÇ̉»¸öÈư½ÇĐÎ

			for (auto& vertex : Vertices) 													//ÉÏ´«Êµ¼ÊÍ¼ĐÎµÄ¶¥µăÊư¾Ư£¬ËüÊÇ̉»¸öÉÏÏÂ·­×ªµÄÈư½ÇĐÎ
				vertex.setY(-vertex.y());
			batch->uploadStaticBuffer(mVertexBuffer.get(), Vertices);

			cmdBuffer->resourceUpdate(batch);
		}

		const QColor clearColor = QColor::fromRgbF(0.0f, 0.0f, 0.0f, 1.0f);
		const QRhiDepthStencilClearValue dsClearValue = { 1.0f,0 };							//Ê¹ÓĂ 0 ÇåÀíÄ£°æ»º³åÇø
		cmdBuffer->beginPass(renderTarget, clearColor, dsClearValue, nullptr);

		cmdBuffer->setGraphicsPipeline(mMaskPipeline.get());
		cmdBuffer->setStencilRef(1);														//ÉèÖĂStencilRefÎª1£¬¸Ă¹ÜÏß»áÔÚÄ£°æ»º³åÇøÉÏ̀î³ä̉»¿éÖµÎª1µÄÈư½ÇĐÎÇøỌ́
		cmdBuffer->setViewport(QRhiViewport(0, 0, mSwapChain->currentPixelSize().width(), mSwapChain->currentPixelSize().height()));
		cmdBuffer->setShaderResources();
		const QRhiCommandBuffer::VertexInput maskVertexInput(mMaskVertexBuffer.get(), 0);
		cmdBuffer->setVertexInput(0, 1, &maskVertexInput);
		cmdBuffer->draw(3);

		cmdBuffer->setGraphicsPipeline(mPipeline.get());
		cmdBuffer->setStencilRef(1);														//ÉèÖĂStencilRefÎª1£¬¸Ă¹ÜÏß»áÓĂ1¸ú¶ÔÓ¦Î»ÖĂµÄÆ¬¶ÎÄ£°æÖµ½øĐĐ±È½Ï£¬ÏàµÈÊ±²Å»áÍ¨¹ưÄ£°æ²âÊÔ£¬̉²¾ÍÊÇ»á½«Æ¬¶Î»æÖÆµ±ÑƠÉ«¸½¼₫ÉÏ
		cmdBuffer->setViewport(QRhiViewport(0, 0, mSwapChain->currentPixelSize().width(), mSwapChain->currentPixelSize().height()));
		cmdBuffer->setShaderResources();
		const QRhiCommandBuffer::VertexInput vertexInput(mVertexBuffer.get(), 0);
		cmdBuffer->setVertexInput(0, 1, &vertexInput);
		cmdBuffer->draw(3);

		cmdBuffer->endPass();
	}
};

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	QRhiHelper::InitParams initParams;
	initParams.backend = QRhi::D3D11;
	TriangleWindow window(initParams);
	window.resize({ 800,600 });
	window.show();
	app.exec();
	return 0;
}
