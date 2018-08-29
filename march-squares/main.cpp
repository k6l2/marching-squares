#pragma once
#if (defined(_WIN32) || defined(__WIN32__))
//have to do this shit otherwise windows defines min/max macros >:|
#define NOMINMAX 1
#include <Windows.h>
#endif
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include <vector>
#include <limits>
std::vector<float> frameTimes(1000, 0.f);
bool prevLeftMousePressed = false;
bool scrollingView = false;
sf::Vector2i scrollMouseScreenOrigin;
sf::Vector2i mouseLocWindow;
sf::Vector2f scrollViewWorldOrigin;
sf::VertexArray vaOrigin(sf::PrimitiveType::Lines, 4);
void drawVertexArray(sf::RenderWindow& window, 
	const sf::VertexArray& va,
	const sf::Transform& transform = sf::Transform::Identity)
{
	sf::VertexArray vaPrime(va);
	for (size_t i = 0; i < va.getVertexCount(); i++)
	{
		vaPrime[i].position = transform.transformPoint(va[i].position);
	}
	window.draw(vaPrime);
}
void handleInput(const sf::RenderWindow& window)
{
	if (!window.hasFocus() || ImGui::IsAnyItemFocused())
	{
		return;
	}
	mouseLocWindow = sf::Mouse::getPosition(window);
	const sf::Vector2f mouseLocWorld = window.mapPixelToCoords(mouseLocWindow);
	const bool isLeftMousePressed = 
		sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	if (isLeftMousePressed)
	{
		scrollingView = true;
		if (!prevLeftMousePressed)
		{
			scrollMouseScreenOrigin = mouseLocWindow;
			scrollViewWorldOrigin = window.getView().getCenter();
		}
	}
	else
	{
		scrollingView = false;
	}
	prevLeftMousePressed = isLeftMousePressed;
}
void step(sf::RenderWindow& window, const sf::Time& frameDelta)
{
	if (scrollingView && !ImGui::IsAnyItemFocused() && 
		!ImGui::IsAnyItemHovered() && !ImGui::IsMouseHoveringAnyWindow())
	{
		const sf::Vector2i scrollMouseScreenDelta = 
			mouseLocWindow - scrollMouseScreenOrigin;
		const sf::Vector2f worldDelta{
			float(scrollMouseScreenDelta.x),
			float(scrollMouseScreenDelta.y) };
		sf::View newView = window.getView();
		newView.setCenter(scrollViewWorldOrigin - worldDelta);
		window.setView(newView);
	}
	// frame time monitor widget //
	frameTimes.push_back(float(frameDelta.asMicroseconds()));
	frameTimes.erase(frameTimes.begin());
	ImGui::Begin("Frame Microseconds", nullptr, 
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
}
void draw(sf::RenderWindow& window)
{
	drawVertexArray(window, vaOrigin, sf::Transform{}.scale({ 100.f,100.f }));
}
int main()
{
	static const float MIN_FPS = 10.f;
	static const sf::Time MAX_FRAME_DELTA = sf::seconds(1.f) / MIN_FPS;
	sf::RenderWindow window(sf::VideoMode(1024, 768), "SFML window");
#if (defined(_WIN32) || defined(__WIN32__))
	//if (bottomLeftConsole)
	{
		HWND consoleWindow = GetConsoleWindow();
		if (consoleWindow != NULL)
		{
			RECT consoleRect;
			RECT desktopRect;
			if (GetWindowRect(consoleWindow, &consoleRect) &&
				SystemParametersInfo(SPI_GETWORKAREA, 0, &desktopRect, 0))
			{
				const LONG consoleW = consoleRect.right - consoleRect.left;
				const LONG consoleH = consoleRect.bottom - consoleRect.top;
				MoveWindow(consoleWindow, desktopRect.left,
					desktopRect.bottom - consoleH,
					consoleW, consoleH, true);
			}
		}
	}
#endif
	ImGui::SFML::Init(window);
	sf::Clock clockFrame;
	// build the vertex array for the origin //
	vaOrigin[0].position = { 0.f, 0.f };
	vaOrigin[0].color = sf::Color::Red;
	vaOrigin[1].position = { 1.f, 0.f };
	vaOrigin[1].color = sf::Color::Red;
	vaOrigin[2].position = { 0.f, 0.f };
	vaOrigin[2].color = sf::Color::Blue;
	vaOrigin[3].position = { 0.f, 1.f };
	vaOrigin[3].color = sf::Color::Blue;
	// center the camera on the origin //
	sf::View newView = window.getDefaultView();
	newView.setCenter({ 0.f,0.f });
	window.setView(newView);
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
		const sf::Time time = clockFrame.restart();
		ImGui::SFML::Update(window, time);
		window.clear();
		handleInput(window);
		step(window, time);
		draw(window);
		ImGui::SFML::Render(window);
		window.display();
	}
	return EXIT_SUCCESS;
}
