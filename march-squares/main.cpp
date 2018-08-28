#pragma once
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include <vector>
int main()
{
	sf::RenderWindow window(sf::VideoMode(800, 600), "SFML window");
	ImGui::SFML::Init(window);
	sf::Clock clockImgui;
	std::vector<float> frameTimes(500, 0.f);
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			ImGui::SFML::ProcessEvent(event);
			switch (event.type)
			{
			case sf::Event::Closed:
				window.close();
				break;
			}
		}
		const sf::Time time = clockImgui.restart();
		ImGui::SFML::Update(window, time);
		window.clear();
		/// TESTING ///////////////////////////////////////////////////////////
		frameTimes.push_back(float(time.asMilliseconds()));
		frameTimes.erase(frameTimes.begin());
		ImGui::Begin("Frame Milliseconds", nullptr, 
			ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::PlotLines("",
			[](void* data, int idx)->float 
			{
				return (*static_cast<std::vector<float>*>(data))[idx]; 
			},
			&frameTimes, frameTimes.size(),
			0, nullptr, std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(), { 400, 150 });
		ImGui::Text("Min=%i", 
			int(*std::min_element(frameTimes.begin(), frameTimes.end())));
		ImGui::SameLine();
		ImGui::Text("Max=%i", 
			int(*std::max_element(frameTimes.begin(), frameTimes.end())));
		ImGui::End();
		/// ///////////////////////////////////////////////////////////////////
		ImGui::SFML::Render(window);
		window.display();
	}
	return EXIT_SUCCESS;
}
