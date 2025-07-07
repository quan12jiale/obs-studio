#include <QApplication>
#include "Render/RHI/QRhiWidget.h"
#include "Render/RHI/QRhiWindow.h"

class ExampleRhiWidget : public QRhiWidget {
public:
	ExampleRhiWidget() {
		setDebugLayer(true);
	}

	void initialize(QRhi* rhi, QRhiTexture* outputTexture) override {
		if (mRhi != rhi) {
			mRenderTarget.reset();
			mRenderPassDesc.reset();
			mDSBuffer.reset();
		}
		else if (mOutputTexture != outputTexture) {
			mRenderTarget.reset();
			mRenderPassDesc.reset();
		}

		mRhi = rhi;
		mOutputTexture = outputTexture;

		if (!mDSBuffer) {
			mDSBuffer.reset(mRhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, mOutputTexture->pixelSize()));
			mDSBuffer->create();
		}
		else if (mDSBuffer->pixelSize() != mOutputTexture->pixelSize()) {
			mDSBuffer->setPixelSize(mOutputTexture->pixelSize());
			mDSBuffer->create();
		}

		if (!mRenderTarget) {
			mRenderTarget.reset(mRhi->newTextureRenderTarget({ { mOutputTexture }, mDSBuffer.data() }));
			mRenderPassDesc.reset(mRenderTarget->newCompatibleRenderPassDescriptor());
			mRenderTarget->setRenderPassDescriptor(mRenderPassDesc.data());
			mRenderTarget->create();
		}
	}
	void render(QRhiCommandBuffer* cb) override {
		const QColor clearColor = QColor::fromRgbF(1.0f, 0.0f, 0.0f, 1.0f);
		const QRhiDepthStencilClearValue dsClearValue = { 1.0f,0 };
		cb->beginPass(mRenderTarget.data(), clearColor, dsClearValue);
		cb->endPass();
	}

protected:
	QRhi* mRhi = nullptr;
	QRhiTexture* mOutputTexture = nullptr;
	QScopedPointer<QRhiRenderBuffer> mDSBuffer;
	QScopedPointer<QRhiTextureRenderTarget> mRenderTarget;
	QScopedPointer<QRhiRenderPassDescriptor> mRenderPassDesc;
};

class ExampleRhiWindow : public QRhiWindow {
public:
	ExampleRhiWindow(QRhiHelper::InitParams inInitParams) :QRhiWindow(inInitParams) {}
protected:
	virtual void onRenderTick() override {
		QRhiRenderTarget* currentRenderTarget = mSwapChain->currentFrameRenderTarget();
		QRhiCommandBuffer* cmdBuffer = mSwapChain->currentFrameCommandBuffer();

		const QColor clearColor = QColor::fromRgbF(0.0f, 0.0f, 1.0f, 1.0f);
		const QRhiDepthStencilClearValue dsClearValue = { 1.0f,0 };

		cmdBuffer->beginPass(currentRenderTarget, clearColor, dsClearValue);
		cmdBuffer->endPass();
	}
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QRhiHelper::InitParams initParams;
	initParams.backend = QRhi::D3D11;
    ExampleRhiWindow window(initParams);
	window.setTitle("01-RhiWindow");
	window.resize({ 400,400 });
	window.show();

	ExampleRhiWidget widget;
	widget.setWindowTitle("01-RhiWidget");
	widget.setApi(QRhiWidget::D3D11);
	widget.resize({ 400,400 });
	widget.show();

    return app.exec();
}
