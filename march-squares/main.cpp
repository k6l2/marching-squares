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
#include <algorithm>
struct DensityField
{
	std::vector<std::vector<float>> density;
	float spacing;
	float pointRadius;
	float isoThreshold;
} densityField{ {25, std::vector<float>(25, -1.f)}, 100.f, 5.f, 0.f };
std::vector<float> frameTimes(1000, 0.f);
bool prevMouseLeftPressed = false;
bool prevMouseRightPressed = false;
bool scrollingView = false;
bool brushPressed = false;
bool brushErase;
float brushRadius = 50.f;
sf::Vector2i scrollMouseScreenOrigin;
sf::Vector2i mouseLocWindow;
sf::Vector2f scrollViewWorldOrigin;
sf::Vector2f mouseLocWorld;
sf::VertexArray vaOrigin(sf::PrimitiveType::Lines, 4);
float v2fMagnitude(const sf::Vector2f v)
{
	return sqrtf(powf(v.x, 2) + powf(v.y, 2));
}
void drawMarchingSquares(sf::RenderWindow& window)
{
	const size_t sizeY = densityField.density.size();
	if (sizeY < 2)
	{
		return;
	}
	const size_t sizeX = densityField.density[0].size();
	if (sizeX < 2)
	{
		return;
	}
	sf::VertexArray vaTris{ sf::PrimitiveType::Triangles };
	auto lerpDensityVertex = [](const sf::Vector2f& p0, const sf::Vector2f& p1,
		float d0, float d1)->sf::Vector2f
	{
		if (fabsf(densityField.isoThreshold - d0) < 1.e-7f)
		{
			return p0;
		}
		if (fabsf(densityField.isoThreshold - d1) < 1.e-7f)
		{
			return p1;
		}
		if (fabsf(d1 - d0) < 1.e-7f)
		{
			return p0;
		}
		const float percent = (densityField.isoThreshold - d0) / (d1 - d0);
		return p0 + percent * (p1 - p0);
	};
	auto polygonize = [&vaTris, &lerpDensityVertex](
		const sf::Vector2i& i0, const sf::Vector2i& i1, 
		const sf::Vector2i& i2)->void
	{
		uint8_t densityFlags = 0;
		const float d0 = densityField.density[i0.y][i0.x];
		const float d1 = densityField.density[i1.y][i1.x];
		const float d2 = densityField.density[i2.y][i2.x];
		const sf::Vector2f p0{ i0.x*densityField.spacing, i0.y*densityField.spacing };
		const sf::Vector2f p1{ i1.x*densityField.spacing, i1.y*densityField.spacing };
		const sf::Vector2f p2{ i2.x*densityField.spacing, i2.y*densityField.spacing };
		if (d0 > densityField.isoThreshold)
			densityFlags |= 1 << 0;
		if (d1 > densityField.isoThreshold)
			densityFlags |= 1 << 1;
		if (d2 > densityField.isoThreshold)
			densityFlags |= 1 << 2;
		switch (densityFlags)
		{
		case 0: // all of the vertices are OUTSIDE of the isosurface
			break;
		case 1:
			vaTris.append({ p0 });
			vaTris.append({ lerpDensityVertex(p0,p1,d0,d1) });
			vaTris.append({ lerpDensityVertex(p0,p2,d0,d2) });
			break;
		case 2:
			vaTris.append({ p1 });
			vaTris.append({ lerpDensityVertex(p1,p0,d1,d0) });
			vaTris.append({ lerpDensityVertex(p1,p2,d1,d2) });
			break;
		case 3:
			vaTris.append({ p0 });
			vaTris.append({ lerpDensityVertex(p1,p2,d1,d2) });
			vaTris.append({ lerpDensityVertex(p0,p2,d0,d2) });
			vaTris.append({ p0 });
			vaTris.append({ p1 });
			vaTris.append({ lerpDensityVertex(p1,p2,d1,d2) });
			break;
		case 4:
			vaTris.append({ p2 });
			vaTris.append({ lerpDensityVertex(p2,p0,d2,d0) });
			vaTris.append({ lerpDensityVertex(p1,p2,d1,d2) });
			break;
		case 5:
			vaTris.append({ p0 });
			vaTris.append({ lerpDensityVertex(p0,p1,d0,d1) });
			vaTris.append({ lerpDensityVertex(p1,p2,d1,d2) });
			vaTris.append({ p2 });
			vaTris.append({ p0 });
			vaTris.append({ lerpDensityVertex(p1,p2,d1,d2) });
			break;
		case 6:
			vaTris.append({ p1 });
			vaTris.append({ p2 });
			vaTris.append({ lerpDensityVertex(p0,p1,d0,d1) });
			vaTris.append({ p2 });
			vaTris.append({ lerpDensityVertex(p2,p0,d2,d0) });
			vaTris.append({ lerpDensityVertex(p0,p1,d0,d1) });
			break;
		case 7:// all of the vertices are INSIDE of the isosurface
			vaTris.append({ p0 });
			vaTris.append({ p1 });
			vaTris.append({ p2 });
			break;
		}
	};
	for (size_t y = 0; y < sizeY - 1; y++)
	{
		for (size_t x = 0; x < sizeX - 1; x++)
		{
			// upper-left triangle
			polygonize(
				{ (int)x    , (int)y }, 
				{ (int)x + 1, (int)y + 1 }, 
				{ (int)x    , (int)y + 1 });
			// lower-right triangle
			polygonize(
				{ (int)x    , (int)y }, 
				{ (int)x + 1, (int)y }, 
				{ (int)x + 1, (int)y + 1 });
		}
	}
	window.draw(vaTris);
}
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
	if (!window.hasFocus() || ImGui::IsAnyItemFocused() ||
		ImGui::IsAnyItemHovered() || ImGui::IsMouseHoveringAnyWindow())
	{
		return;
	}
	mouseLocWindow = sf::Mouse::getPosition(window);
	mouseLocWorld = window.mapPixelToCoords(mouseLocWindow);
	const bool isLeftMousePressed = 
		sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	const bool isRightMousePressed = 
		sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);
	brushPressed = isLeftMousePressed;
	if (brushPressed)
	{
		brushErase = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);
	}
	if (isRightMousePressed)
	{
		scrollingView = true;
		if (!prevMouseRightPressed)
		{
			scrollMouseScreenOrigin = mouseLocWindow;
			scrollViewWorldOrigin = window.getView().getCenter();
		}
	}
	else
	{
		scrollingView = false;
	}
	prevMouseLeftPressed  = isLeftMousePressed;
	prevMouseRightPressed = isRightMousePressed;
}
void step(sf::RenderWindow& window, const sf::Time& frameDelta)
{
	if (brushPressed && !ImGui::IsAnyItemFocused() &&
		!ImGui::IsAnyItemHovered() && !ImGui::IsMouseHoveringAnyWindow())
	{
		// find all the density that we're touching
		sf::FloatRect brushAabb{
			(mouseLocWorld.x - brushRadius)/densityField.spacing,
			(mouseLocWorld.y + brushRadius)/densityField.spacing,
			(brushRadius * 2)/densityField.spacing,
			(brushRadius * 2)/densityField.spacing };
		const int brushBottom = int(brushAabb.top - brushAabb.height);
		const int brushRight  = int(brushAabb.left + brushAabb.width);
		ImGui::Begin("DEBUG - brush", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("bottom=%i top=%i left=%i right=%i",
			brushBottom, int(brushAabb.top), int(brushAabb.left), brushRight);
		for (int y = brushBottom; y <= int(brushAabb.top); y++)
		{
			for (int x = int(brushAabb.left); x <= brushRight; x++)
			{
				if (x < 0 || y < 0 || 
					y >= int(densityField.density.size()) ||
					x >= int(densityField.density[0].size()))
				{
					ImGui::Text("[%i,%i] out of bounds!",x,y);
					continue;
				}
				const sf::Vector2f fieldPointLocWorld{
					x*densityField.spacing,
					y*densityField.spacing };
				const sf::Vector2f toMouseVec =
					mouseLocWorld - fieldPointLocWorld;
				const float toMouseMag = v2fMagnitude(toMouseVec);
				if (toMouseMag > brushRadius + densityField.pointRadius)
				{
					ImGui::Text("[%i,%i] outside brush!", x, y);
					continue;
				}
				///ImGui::Text("toMouseMag=%f", toMouseMag);
				const float ratio = std::min(toMouseMag / 
					(brushRadius + densityField.pointRadius), 1.f);
				if (brushErase)
				{
					densityField.density[y][x] =
						std::min(densityField.density[y][x],
							-1.f + ratio*2);
				}
				else
				{
					densityField.density[y][x] = 
						std::max(densityField.density[y][x], 
							1.f - ratio*2);
					ImGui::Text("[%i,%i] density=%f", 
						x,y, densityField.density[y][x]);
				}
			}
		}
		ImGui::End();
	}
	if (scrollingView && !ImGui::IsAnyItemFocused() && 
		!ImGui::IsAnyItemHovered() && !ImGui::IsMouseHoveringAnyWindow())
	{
		const sf::Vector2i scrollMouseScreenDelta = 
			mouseLocWindow - scrollMouseScreenOrigin;
		const sf::Vector2f worldDelta{
			float(scrollMouseScreenDelta.x),
			float(scrollMouseScreenDelta.y)*-1 };
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
	// adjust density field widget //
	ImGui::Begin("Density Field");
	int sizeY = densityField.density.size();
	if (sizeY > 0)
	{
		int sizeX = densityField.density[0].size();
		if (ImGui::DragInt("sizeX", &sizeX, 1.f,
			0, std::numeric_limits<int>::max()))
		{
			for (auto& densityY : densityField.density)
			{
				densityY.resize(sizeX);
			}
		}
	}
	if (ImGui::DragInt("sizeY", &sizeY, 1.f,
		0, std::numeric_limits<int>::max()))
	{
		densityField.density.resize(sizeY);
	}
	ImGui::DragFloat("spacing", &densityField.spacing);
	ImGui::DragFloat("pointRadius", &densityField.pointRadius);
	ImGui::End();
	// brush tool widget //
	ImGui::Begin("Brush Widget");
	ImGui::DragFloat("radius", &brushRadius);
	ImGui::End();
}
void draw(sf::RenderWindow& window)
{
	drawVertexArray(window, vaOrigin, sf::Transform{}.scale({ 100.f,100.f }));
	drawMarchingSquares(window);
	// draw the density field points //
	sf::CircleShape densityCircle{ densityField.pointRadius };
	densityCircle.setOrigin(
		{ densityField.pointRadius,densityField.pointRadius });
	densityCircle.setOutlineThickness(0.f);
	for (size_t y = 0; y < densityField.density.size(); y++)
	{
		for (size_t x = 0; x < densityField.density[y].size(); x++)
		{
			densityCircle.setPosition(
				x*densityField.spacing, y*densityField.spacing);
			if (densityField.density[y][x] > 0)
			{
				densityCircle.setFillColor(sf::Color::Green);
			}
			else if (densityField.density[y][x] < 0)
			{
				densityCircle.setFillColor(sf::Color::Red);
			}
			else
			{
				densityCircle.setFillColor(sf::Color::White);
			}
			window.draw(densityCircle);
		}
	}
	// draw the brush //
	sf::CircleShape circleBrush(brushRadius);
	circleBrush.setOrigin(brushRadius, brushRadius);
	circleBrush.setFillColor(sf::Color::Transparent);
	circleBrush.setOutlineThickness(1.f);
	circleBrush.setOutlineColor(sf::Color::White);
	circleBrush.setPosition(mouseLocWorld);
	window.draw(circleBrush);
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
	// init the density field to some reasonable values //
	///densityField.density.resize(25);
	///for (auto& densityY : densityField.density)
	///{
	///	densityY.resize(25);
	///}
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
	newView.setSize(newView.getSize().x, newView.getSize().y*-1);
	window.setView(newView);
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			ImGui::SFML::ProcessEvent(event);
			switch (event.type)
			{
			case sf::Event::EventType::Closed:
				window.close();
				break;
			case sf::Event::EventType::KeyPressed:
				switch (event.key.code)
				{
				case sf::Keyboard::Key::Escape:
					window.close();
					break;
				}
				break;
			case sf::Event::EventType::Resized:
				newView = window.getView();
				newView.setSize(
					(float)event.size.width, event.size.height*-1.f);
				window.setView(newView);
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
