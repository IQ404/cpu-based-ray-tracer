/*****************************************************************//**
 * \file   mainloop.cpp
 * \brief  The main rendering function that is called for every frame
 * 
 * \author Xiaoyang Liu (built upon the Walnut GUI template: https://github.com/TheCherno/WalnutAppTemplate)
 * \date   May 2023
 *********************************************************************/

#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Timer.h"

#include "Camera.h"
#include "Renderer.h"

class CSC8599Layer : public Walnut::Layer
{
	float duration_per_frame = 0.0f;
	bool real_time = false;
	Renderer renderer;
	Camera camera{ 35.0f, 0.1f, 100.0f };
	uint32_t viewport_width = 0;
	uint32_t viewport_height = 0;

public:
	CSC8599Layer()
	{
		
	}

	virtual void OnUpdate(float dt) override
	{
		if (real_time)
		{
			camera.UpdateCamera(dt);
		}
	}

	virtual void OnUIRender() override
	{
//----------------------------------------------------------------------------------------------------------------------------------------

// Viewport:

		ImGui::Begin("Viewport");

		// dimension (in number of pixels):
		
		viewport_width = ImGui::GetContentRegionAvail().x;
		viewport_height = ImGui::GetContentRegionAvail().y;

		auto final_image = renderer.GetFinalImage();
		if (final_image)
		{
			ImGui::Image(final_image->GetDescriptorSet(), { (float)final_image->GetWidth(), (float)final_image->GetHeight() }, { 0,1 }, { 1,0 });		// display the image
			/*
			Note for the last two parameters:
			change uv0 (0,0) which is originally at the top left to (0,1) i.e. bottom left; change uv1 (1,1) which is originally at the bottom right to (0,1) i.e. top right.
			*/
		}

		// TODO: explain why we cannot call ImGui::GetContentRegionAvail() here:
		/*viewport_width = ImGui::GetContentRegionAvail().x;
		viewport_height = ImGui::GetContentRegionAvail().y;*/

		ImGui::End();

//----------------------------------------------------------------------------------------------------------------------------------------
		
// Control Panel:

		ImGui::Begin("Control Panel");

		ImGui::Text("%.0f FPS", 1000.0f/duration_per_frame);	// Note that this will print inf if duration_per_frame == 0
		ImGui::Text("%.0f ms", duration_per_frame);

		ImGui::Separator();

		ImGui::Text("Current Settings:");
		ImGui::Text("real time rendering    %.0f", (float)real_time);
		ImGui::Text("Clamp immediately during intermediate outputs    %.0f", (float)renderer.GetSettings().immediate_clamping);
		ImGui::Text("JointBilateralFiltering_15    %.0f", (float)renderer.GetSettings().using_JointBilateralFiltering_15);
		ImGui::Text("JointBilateralFiltering_33    %.0f", (float)renderer.GetSettings().using_JointBilateralFiltering_33);
		ImGui::Text("JointBilateralFiltering_65    %.0f", (float)renderer.GetSettings().using_JointBilateralFiltering_65);
		ImGui::Text("temporal_kernel_7    %.0f", (float)renderer.GetSettings().using_temporal_kernel_7);
		ImGui::Text("temporal_kernel_15    %.0f", (float)renderer.GetSettings().using_temporal_kernel_15);
		ImGui::Text("temporal_kernel_33    %.0f", (float)renderer.GetSettings().using_temporal_kernel_33);
		ImGui::Text("Temporal_Variance_Tolerance_1    %.0f", (float)renderer.GetSettings().using_temporal_variance_tolerance_1);
		ImGui::Text("Temporal_Variance_Tolerance_2    %.0f", (float)renderer.GetSettings().using_temporal_variance_tolerance_2);
		ImGui::Text("Temporal_Variance_Tolerance_3    %.0f", (float)renderer.GetSettings().using_temporal_variance_tolerance_3);
		ImGui::Text("Current_Frame_Weighting_0.05    %.0f", (float)renderer.GetSettings().using_temporal_current_frame_weighting_5);
		ImGui::Text("Current_Frame_Weighting_0.1    %.0f", (float)renderer.GetSettings().using_temporal_current_frame_weighting_10);
		ImGui::Text("Current_Frame_Weighting_0.2    %.0f", (float)renderer.GetSettings().using_temporal_current_frame_weighting_20);
		ImGui::Text("Current_Frame_Weighting_0.5    %.0f", (float)renderer.GetSettings().using_temporal_current_frame_weighting_50);

		ImGui::Separator();

		if (ImGui::Button("Render in Real-Time"))
		{
			real_time = true;
		}

		ImGui::Separator();

		if (ImGui::Button("Enable immediate clamping"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().immediate_clamping = true;
		}
		if (ImGui::Button("Disable immediate clamping"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().immediate_clamping = false;
		}

		ImGui::Separator();

		ImGui::Text("Spatial denoising:");

		if (ImGui::Button("Disable Joint Bilateral Filtering"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_JointBilateralFiltering = true;
			renderer.GetSettings().using_JointBilateralFiltering_15 = false;
			renderer.GetSettings().using_JointBilateralFiltering_33 = false;
			renderer.GetSettings().using_JointBilateralFiltering_65 = false;
		}
		if (ImGui::Button("Joint Bilateral Filtering with kernel size: 15 pixels"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_JointBilateralFiltering = false;
			renderer.GetSettings().using_JointBilateralFiltering_15 = true;
			renderer.GetSettings().using_JointBilateralFiltering_33 = false;
			renderer.GetSettings().using_JointBilateralFiltering_65 = false;
		}
		if (ImGui::Button("Joint Bilateral Filtering with kernel size: 33 pixels"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_JointBilateralFiltering = false;
			renderer.GetSettings().using_JointBilateralFiltering_15 = false;
			renderer.GetSettings().using_JointBilateralFiltering_33 = true;
			renderer.GetSettings().using_JointBilateralFiltering_65 = false;
		}
		if (ImGui::Button("Joint Bilateral Filtering with kernel size: 65 pixels"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_JointBilateralFiltering = false;
			renderer.GetSettings().using_JointBilateralFiltering_15 = false;
			renderer.GetSettings().using_JointBilateralFiltering_33 = false;
			renderer.GetSettings().using_JointBilateralFiltering_65 = true;
		}

		ImGui::Separator();

		ImGui::Text("Temporal denoising:");

		if (ImGui::Button("Disable Temporal Filtering"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_TemporalFiltering = true;
		}
		if (ImGui::Button("Temporal Filtering with kernel size: 7 pixels"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_TemporalFiltering = false;
			renderer.GetSettings().using_temporal_kernel_7 = true;
		}
		if (ImGui::Button("Temporal Filtering with kernel size: 15 pixels"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_TemporalFiltering = false;
			renderer.GetSettings().using_temporal_kernel_7 = false;
			renderer.GetSettings().using_temporal_kernel_15 = true;
		}
		if (ImGui::Button("Temporal Filtering with kernel size: 33 pixels"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_TemporalFiltering = false;
			renderer.GetSettings().using_temporal_kernel_7 = false;
			renderer.GetSettings().using_temporal_kernel_15 = false;
			renderer.GetSettings().using_temporal_kernel_33 = true;
		}
		if (ImGui::Button("Temporal Variance Tolerance = 1"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_TemporalFiltering = false;
			renderer.GetSettings().using_temporal_variance_tolerance_1 = true;
		}
		if (ImGui::Button("Temporal Variance Tolerance = 2"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_TemporalFiltering = false;
			renderer.GetSettings().using_temporal_variance_tolerance_1 = false;
			renderer.GetSettings().using_temporal_variance_tolerance_2 = true;
		}
		if (ImGui::Button("Temporal Variance Tolerance = 3"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_TemporalFiltering = false;
			renderer.GetSettings().using_temporal_variance_tolerance_1 = false;
			renderer.GetSettings().using_temporal_variance_tolerance_2 = false;
			renderer.GetSettings().using_temporal_variance_tolerance_3 = true;
		}
		if (ImGui::Button("Current Frame Weighting: 5%"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_TemporalFiltering = false;
			renderer.GetSettings().using_temporal_current_frame_weighting_5 = true;
		}
		if (ImGui::Button("Current Frame Weighting: 10%"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_TemporalFiltering = false;
			renderer.GetSettings().using_temporal_current_frame_weighting_5 = false;
			renderer.GetSettings().using_temporal_current_frame_weighting_10 = true;
		}
		if (ImGui::Button("Current Frame Weighting: 20%"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_TemporalFiltering = false;
			renderer.GetSettings().using_temporal_current_frame_weighting_5 = false;
			renderer.GetSettings().using_temporal_current_frame_weighting_10 = false;
			renderer.GetSettings().using_temporal_current_frame_weighting_20 = true;
		}
		if (ImGui::Button("Current Frame Weighting: 50%"))
		{
			renderer.RestartTemporal();
			renderer.GetSettings().disable_TemporalFiltering = false;
			renderer.GetSettings().using_temporal_current_frame_weighting_5 = false;
			renderer.GetSettings().using_temporal_current_frame_weighting_10 = false;
			renderer.GetSettings().using_temporal_current_frame_weighting_20 = false;
			renderer.GetSettings().using_temporal_current_frame_weighting_50 = true;
		}

		ImGui::Separator();

		if (ImGui::Button("Render Offline"))
		{
			real_time = false;
			renderer.Reaccumulate();
			Render();
		}

		ImGui::Separator();

		ImGui::End();

//----------------------------------------------------------------------------------------------------------------------------------------

// Miscellaneous:
		
		//ImGui::ShowDemoWindow();
		
//----------------------------------------------------------------------------------------------------------------------------------------

		if (real_time)
		{
			Render();
		}
	}

	void Render()
	{
		Walnut::Timer frame_timer;	// this timer starts counting from here

		renderer.ResizeViewport(viewport_width, viewport_height);
		camera.ResizeViewport(viewport_width, viewport_height);
		renderer.Render(camera);

		duration_per_frame = frame_timer.ElapsedMillis();
	}
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "8599 Ray Tracer";

	Walnut::Application* app = new Walnut::Application(spec);	// Vulkan is initialized AFTER this point
	app->PushLayer<CSC8599Layer>();		// OnUIRender() will be called every loop after this layer has been pushed.
	return app;
}