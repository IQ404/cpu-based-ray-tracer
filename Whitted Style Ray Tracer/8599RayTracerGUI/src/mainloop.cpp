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
			if (camera.UpdateCamera(dt))
			{
				renderer.Reaccumulate();
			}
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

		ImGui::End();

//----------------------------------------------------------------------------------------------------------------------------------------
		
// Control Panel:

		ImGui::Begin("Control Panel");

		ImGui::Text("%.0f FPS", 1000.0f/duration_per_frame);	// Note that this will print inf if duration_per_frame == 0
		ImGui::Text("%.0f ms", duration_per_frame);

		ImGui::Separator();

		if (ImGui::Button("Render in Real-Time"))
		{
			real_time = true;
		}

		ImGui::Checkbox("Temporal Accumulation", &renderer.GetSettings().accumulating);

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