#include <QApplication>
//#include "QEngineApplication.h"
#include "Render/RHI/QRhiWindow.h"

static float VertexData[] = {										//顶点数据
	//position(xy)		color(rgba)
	 0.0f,  -0.5f,		1.0f, 0.0f, 0.0f, 1.0f,
	-0.5f,   0.5f,		0.0f, 1.0f, 0.0f, 1.0f,
	 0.5f,   0.5f,		0.0f, 0.0f, 1.0f, 1.0f,
};
static const int ncols = 6;

class TriangleWindow : public QRhiWindow {
private:
	QRhiSignal mSigInit;										//用于初始化的信号
	QRhiSignal mSigSubmit;										//用于提交资源的信号
	QScopedPointer<QRhiBuffer> mVertexBuffer;						//顶点缓冲区
	QScopedPointer<QRhiShaderResourceBindings> mShaderBindings;		//描述符集布局绑定
	QScopedPointer<QRhiGraphicsPipeline> mPipeline;					//图形渲染管线
	// test:2、绘制矩形，圆形或者其他多边形图像。
	QVector<float> vertex_data_;

public:
	TriangleWindow(QRhiHelper::InitParams inInitParams) 
		: QRhiWindow(inInitParams) {
		mSigInit.request();			//请求初始化
		mSigSubmit.request();		//请求提交资源
		setupCircle();
	}
protected:
	virtual void onRenderTick() override {
		if (mSigInit.ensure()) {	//初始化资源
			mVertexBuffer.reset(mRhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(VertexData)));
			//mVertexBuffer.reset(mRhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
							    //vertex_data_.size() * static_cast<quint32>(sizeof(float))));
			mVertexBuffer->create();
			QRhiVertexInputLayout inputLayout;
			inputLayout.setBindings({
				QRhiVertexInputBinding(6 * sizeof(float))		//定义每个VertexBuffer，单组顶点数据的跨度，这里是 6 * sizeof(float)，可以当作是GPU会从Buffer0（0是Index）读取 6 * sizeof(float) 传给 Vertex Shader
			});

			inputLayout.setAttributes({
				QRhiVertexInputAttribute(0, 0 , QRhiVertexInputAttribute::Float2, 0),					// 从每组顶点数据的位置 0 开始作为 Location0（Float2） 的起始地址
				QRhiVertexInputAttribute(0, 1 , QRhiVertexInputAttribute::Float4, sizeof(float) * 2),	// 从每组顶点数据的位置 sizeof(float) * 2 开始作为 Location1（Float4） 的起始地址
			});

			mPipeline.reset(mRhi->newGraphicsPipeline());

			mPipeline->setVertexInputLayout(inputLayout);

			mShaderBindings.reset(mRhi->newShaderResourceBindings());															//创建绑定
			mShaderBindings->create();
			mPipeline->setShaderResourceBindings(mShaderBindings.get());														//绑定到流水线
			mPipeline->setSampleCount(mSwapChain->sampleCount());
			mPipeline->setRenderPassDescriptor(mSwapChainPassDesc.get());

			QShader vs = QRhiHelper::newShaderFromCode(QShader::VertexStage, R"(#version 440
				layout(location = 0) in vec2 position;		//这里需要与上面的inputLayout 对应
				layout(location = 1) in vec4 color;

				layout (location = 0) out vec4 vColor;		//输出变量到 fragment shader 中，这里的location是out的，而不是in

				out gl_PerVertex { 
					vec4 gl_Position;
				};

				void main(){
					gl_Position = vec4(position,0.0f,1.0f);
					vColor = color;
				}
			)");
			Q_ASSERT(vs.isValid());

			QShader fs = QRhiHelper::newShaderFromCode(QShader::FragmentStage, R"(#version 440
				layout (location = 0) in vec4 vColor;		//上一阶段的out变成了这一阶段的in
				layout (location = 0) out vec4 fragColor;	//片段着色器输入
				void main(){
					fragColor = vColor;
				}
			)");
			Q_ASSERT(fs.isValid());

			mPipeline->setShaderStages({
				QRhiShaderStage(QRhiShaderStage::Vertex, vs),
				QRhiShaderStage(QRhiShaderStage::Fragment, fs)
			});
			// qrhid3d11.cpp文件toD3DTopology函数将QRhiGraphicsPipeline::Topology枚举映射为D3D11_PRIMITIVE_TOPOLOGY枚举
			// D3D不支持QRhiGraphicsPipeline::Topology::TriangleFan枚举
			// test:1、修改 图元 拓扑，绘制点，线。
			//mPipeline->setTopology(QRhiGraphicsPipeline::Topology::Triangles);
			mPipeline->create();
		}

		QRhiRenderTarget* renderTarget = mSwapChain->currentFrameRenderTarget();	//交互链中的当前渲染目标
		QRhiCommandBuffer* cmdBuffer = mSwapChain->currentFrameCommandBuffer();		//交互链中的当前指令缓冲

		if (mSigSubmit.ensure()) {
			QRhiResourceUpdateBatch* batch = mRhi->nextResourceUpdateBatch();		//申请资源操作合批入口
			batch->uploadStaticBuffer(mVertexBuffer.get(), VertexData);				//上传顶点数据
			//batch->uploadStaticBuffer(mVertexBuffer.get(), vertex_data_.data());
			cmdBuffer->resourceUpdate(batch);
		}

		const QColor clearColor = QColor::fromRgbF(0.0f, 0.0f, 0.0f, 1.0f);			//使用该值来清理 渲染目标 中的 颜色附件
		const QRhiDepthStencilClearValue dsClearValue = { 1.0f,0 };					//使用该值来清理 渲染目标 中的 深度和模板附件
		cmdBuffer->beginPass(renderTarget, clearColor, dsClearValue, nullptr);		//开启渲染通道

		cmdBuffer->setGraphicsPipeline(mPipeline.get());							//设置图形渲染管线
		cmdBuffer->setViewport(QRhiViewport(0, 0, mSwapChain->currentPixelSize().width(), mSwapChain->currentPixelSize().height()));		//设置图像的绘制区域
		cmdBuffer->setShaderResources();											//设置描述符集布局绑定，如果不填参数（为nullptr），则会使用渲染管线创建时所使用的描述符集布局绑定
		const QRhiCommandBuffer::VertexInput vertexInput(mVertexBuffer.get(), 0);	//将 mVertexBuffer 绑定到Buffer0，内存偏移值为0
		cmdBuffer->setVertexInput(0, 1, &vertexInput);								
		cmdBuffer->draw(3);															//执行绘制，其中 3 代表着有 3个顶点数据 输入
		//cmdBuffer->draw(vertex_data_.size() / ncols);

		cmdBuffer->endPass();														//结束渲染通道
	}
	void setupCircle() {
		const int numSegments = 100; // 圆的细分数
		const float radius = 1.0f;   // 圆的半径

		for (int i = 0; i <= numSegments; ++i) {
			float angle = 2.0f * M_PI * i / numSegments;
			float x = radius * cos(angle);
			float y = radius * sin(angle);

			float next_angle = 2.0f * M_PI * (i+1) / numSegments;
			float next_x = radius * cos(next_angle);
			float next_y = radius * sin(next_angle);

			vertex_data_.append(0.0f); // 中心点
			vertex_data_.append(0.0f);

			vertex_data_.push_back(1.0f); // r
			vertex_data_.push_back(0.0f); // g
			vertex_data_.push_back(0.0f); // b
			vertex_data_.push_back(1.0f); // a

			vertex_data_.push_back(x);
			vertex_data_.push_back(y);

			vertex_data_.push_back(1.0f); // r
			vertex_data_.push_back(0.0f);           // g
			vertex_data_.push_back(0.0f); // b
			vertex_data_.push_back(1.0f);           // a

			// TODO 其实这里可以用IndexBuffer，就不用生成重复的顶点数据
			vertex_data_.push_back(next_x);
			vertex_data_.push_back(next_y);

			vertex_data_.push_back(1.0f); // r
			vertex_data_.push_back(0.0f); // g
			vertex_data_.push_back(0.0f); // b
			vertex_data_.push_back(1.0f); // a
		}
	}
};

int main(int argc, char **argv)
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
