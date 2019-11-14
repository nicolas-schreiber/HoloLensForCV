#pragma once
#include "MainPage.g.h"

#include <ppltasks.h>
#include <collection.h>

namespace ComputeOnDesktopYolo
{
	namespace YoloModelIO
	{
		public ref class Input sealed
		{
		public:
			property ::Windows::AI::MachineLearning::ImageFeatureValue^ image; // shape(-1,3,416,416)
		};

		public ref class Output sealed
		{
		public:
			property ::Windows::AI::MachineLearning::TensorFloat^ grid; // shape(-1,125,13,13)
		};

		public ref class Model sealed
		{
		private:
			::Windows::AI::MachineLearning::LearningModel^ m_model;
			::Windows::AI::MachineLearning::LearningModelSession^ m_session;
			::Windows::AI::MachineLearning::LearningModelBinding^ m_binding;
		public:
			property ::Windows::AI::MachineLearning::LearningModel^ LearningModel { ::Windows::AI::MachineLearning::LearningModel^ get() { return m_model; } }
			property ::Windows::AI::MachineLearning::LearningModelSession^ LearningModelSession { ::Windows::AI::MachineLearning::LearningModelSession^ get() { return m_session; } }
			property ::Windows::AI::MachineLearning::LearningModelBinding^ LearningModelBinding { ::Windows::AI::MachineLearning::LearningModelBinding^ get() { return m_binding; } }
			static ::Windows::Foundation::IAsyncOperation<Model^>^ CreateFromStreamAsync(::Windows::Storage::Streams::IRandomAccessStreamReference^ stream, ::Windows::AI::MachineLearning::LearningModelDeviceKind deviceKind /*= ::Windows::AI::MachineLearning::LearningModelDeviceKind::Default*/)
			{
				return ::concurrency::create_async([stream, deviceKind] {
					return ::concurrency::create_task(::Windows::AI::MachineLearning::LearningModel::LoadFromStreamAsync(stream))
						.then([deviceKind](::Windows::AI::MachineLearning::LearningModel^ learningModel) {
						Model^ model = ref new Model();
						model->m_model = learningModel;
						::Windows::AI::MachineLearning::LearningModelDevice^ device = ref new ::Windows::AI::MachineLearning::LearningModelDevice(deviceKind);
						model->m_session = ref new ::Windows::AI::MachineLearning::LearningModelSession(model->m_model, device);
						model->m_binding = ref new ::Windows::AI::MachineLearning::LearningModelBinding(model->m_session);
						return model;
					});
				});
			}
			::Windows::Foundation::IAsyncOperation<Output^>^ EvaluateAsync(Input^ input)
			{
				if (this->m_model == nullptr) throw ref new Platform::InvalidArgumentException();
				return ::concurrency::create_async([this, input] {
					return ::concurrency::create_task([this, input]() {
						m_binding->Bind("image", input->image);
						return ::concurrency::create_task(this->m_session->EvaluateAsync(m_binding, L""));
					}).then([](::Windows::AI::MachineLearning::LearningModelEvaluationResult^ evalResult) {
						Output^ output = ref new Output();
						output->grid = static_cast<::Windows::AI::MachineLearning::TensorFloat^>(evalResult->Outputs->Lookup("grid"));
						return output;
					});
				});
			}
		};
	}

}
