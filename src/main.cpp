#include "Board.h"

const float viewLength = 512.0f;
const unsigned int windowLength = 984;
sf::Color backgroundColor = sf::Color(20, 20, 20, 0);

int main()
{
	//sf::RenderWindow window(sf::VideoMode(windowLength, windowLength), "Chess | C++ | SFML", sf::Style::Default);
	sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Chess | C++ | SFML", sf::Style::Fullscreen);
	window.setFramerateLimit(60);
	window.setKeyRepeatEnabled(false);

	sf::View view(sf::Vector2f(viewLength / 2, viewLength / 2), sf::Vector2f(viewLength, viewLength));

	Board chessBoard(boardThemes::random, "pixel");
	chessBoard.resize(view, viewLength, window.getSize());

	sf::Event e;
	sf::Vector2i mousePos;

	while (window.isOpen())
	{
		while (window.pollEvent(e))
		{

			if (e.type == sf::Event::Closed)
				window.close();


			if (e.type == sf::Event::Resized)
				chessBoard.resize(view, viewLength, window.getSize());


			if (e.type == sf::Event::KeyPressed)
			{
				if (e.key.code == sf::Keyboard::Q || e.key.code == sf::Keyboard::Escape)
					window.close();

				if (e.key.code == sf::Keyboard::F)
					chessBoard.flip();

				if (e.key.code == sf::Keyboard::B)
					chessBoard.randomBoardTheme();

				if (e.key.code == sf::Keyboard::R)
					chessBoard.rgbBoardTheme();

				if (e.key.code == sf::Keyboard::P)
					chessBoard.randomPieceTheme();

				if (e.key.code == sf::Keyboard::H)
					chessBoard.togglePieceVisibilty();

				if (e.key.code == sf::Keyboard::L)
					chessBoard.toggleLabelsVisibility();

				if (e.key.code == sf::Keyboard::T)
					chessBoard.toggleThreatsVisibility();

				if (e.key.code == sf::Keyboard::N)
					chessBoard.toggleNotationVisibility();

				if (e.key.code == sf::Keyboard::A)
					chessBoard.toggleNotationAlignment();
			}


			if (e.type == sf::Event::MouseButtonPressed || e.type == sf::Event::MouseButtonReleased)
			{
				if (e.mouseButton.button == sf::Mouse::Left)
				{
					mousePos = sf::Mouse::getPosition(window);
					chessBoard.movePiece(mousePos, window.getSize(), sf::Mouse::isButtonPressed(sf::Mouse::Left));
				}
			}

		}

		window.clear(backgroundColor);
		window.setView(view);
		chessBoard.draw(window);
		window.display();
	}

	return 0;
}