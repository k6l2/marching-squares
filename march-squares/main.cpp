#pragma once
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
int main()
{
	sf::RenderWindow window(sf::VideoMode(800, 600), "SFML window");
	ImGui::SFML::Init(window);
	sf::Clock clockImgui;
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
		ImGui::SFML::Update(window, clockImgui.restart());
		window.clear();
		/// TESTING ///////////////////////////////////////////////////////////
		ImGui::Begin("Testing!");
		ImGui::Button("Test Button");
		ImGui::End();
		/// ///////////////////////////////////////////////////////////////////
		ImGui::SFML::Render(window);
		window.display();
	}
	return EXIT_SUCCESS;
}
