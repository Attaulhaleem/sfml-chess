#include "Board.h"
#include <iostream>			// for std::cerr
#include <sstream>

std::ostream& operator<<(std::ostream& stream, const IntPairVec& vec)
{
	for (const IntPair& v : vec)
		stream << v.second << ", " << v.first << "\t";

	stream << std::endl;

	return stream;
}

// ------------------------------------------- PUBLIC FUNCTIONS -------------------------------------------

// Constructor

Board::Board(const boardThemes& boardTheme, const std::string& piecesTheme)
{
	srand(static_cast<unsigned int>(time(NULL)));
	rgbTime = 100;
	hsvColor = { randomFrac * 360, 1.0, 1.0 };
	rgbColor = HSV2RGB(hsvColor);

	squareSize = 64.0f;
	checkmate = false;
	stalemate = false;
	facingWhite = true;
	moveAllowed = false;
	movesVisible = false;
	threatsVisible = false;
	eSquarePos = sf::Vector2u(8, 8);

	selectBoardTheme(boardTheme);
	this->piecesTheme = piecesTheme;
	piecesVisible = true;
	notationVisible = true;
	leftNotation = true;
	labelsVisible = false;

	if (!notationFont.loadFromFile("../Resources/Fonts/Segoe UI Bold.ttf"))
	{
		std::cerr << "Fatal Error! Notation font not loaded! Board::Board()" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	if (!labelFont.loadFromFile("../Resources/Fonts/Segoe UI.ttf"))
	{
		std::cerr << "Fatal Error! Label font not loaded! Board::Board()" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	if (!squareTexture.loadFromFile("../Resources/Textures/marble.jpg"))
	{
		std::cerr << "Fatal Error! Square texture not loaded! Board::Board()" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	//FEN = "rnbqkbnr/8/8/pppppppp/PPPPPPPP/8/8/RNBQKBNR w KQkq - 0 1";
	loadFen();
	getAllThreats();
}


// Drawing

void Board::draw(sf::RenderWindow& window)
{
	if (boardTheme == boardThemes::rgb) updateColors();

	sf::RectangleShape square(sf::Vector2f(squareSize, squareSize));
	square.setTexture(&squareTexture);

	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 8; ++j)
		{
			square.setFillColor(moveAllowed && i == hSquarePos.y && j == hSquarePos.x ? hColor : (i + j) % 2 ? bColor : wColor);
			square.setPosition(j * squareSize, i * squareSize);
			window.draw(square);

			if (White.inCheck && board[i][j] == 1 || Black.inCheck && board[i][j] == -1)
				drawCheck(i, j, window);

			if (notationVisible)	drawNotation(i, j, window);
			if (piecesVisible)		drawPiece(i, j, window);
			if (labelsVisible)		drawLabels(i, j, window);
		}
	}

	if (movesVisible) drawMoves(window);

	if (threatsVisible) drawThreats(window);
}


// Mouse Input

void Board::resize(sf::View& view, const float& viewLength, const sf::Vector2u& windowSize)
{
	float aspectRatio = float(windowSize.x) / float(windowSize.y);

	if (windowSize.x == windowSize.y)
		view.setSize(viewLength, viewLength);
	else if (windowSize.x > windowSize.y)
		view.setSize(viewLength * aspectRatio, viewLength);
	else if (windowSize.y > windowSize.x)
		view.setSize(viewLength, viewLength / aspectRatio);
}

void Board::movePiece(const sf::Vector2i& mousePos, const sf::Vector2u& windowSize, const bool& isMousePressed)
{
	if (isMousePressed && !checkmate && !stalemate)											// if holding down mouse
	{
		hSquarePos = getMouseSquare(mousePos, windowSize);									// find position of selected square
		availableMoves.clear();																// clear the list of available moves

		if (board[hSquarePos.y][hSquarePos.x] &&											// clicked square is not empty
			whiteToMove && board[hSquarePos.y][hSquarePos.x] > 0 ||							// only white pieces move on white turn
			!whiteToMove && board[hSquarePos.y][hSquarePos.x] < 0)							// only black pieces move on black turn
		{
			moveAllowed = true;																// highlight selected square if appropriate
			availableMoves = getPieceMoves(hSquarePos.y, hSquarePos.x);						// get list of moves
			movesVisible = true;															// set to true to show available moves
		}
		else
			moveAllowed = false;															// do not highlight selected square
	}
	else if (moveAllowed)																	// on mouse release if allowed piece was selected
	{
		moveAllowed = false;																// do not highlight new square unless it is correct (set to true below)

		sf::Vector2u newSquarePos = getMouseSquare(mousePos, windowSize);					// get destination square

		for (auto it = availableMoves.begin(); it != availableMoves.end(); ++it)			// loop over all available moves
		{
			if (newSquarePos.x == it->first && newSquarePos.y == it->second)				// if making a legal move
			{
				// special moves

				bool enPassantPlayed = false;
				bool castlingPlayed = false;

				if (isKing(hSquarePos.y, hSquarePos.x) && existsInVec(*it, castlingSquares))
				{
					castle(hSquarePos, newSquarePos);
					castlingPlayed = true;
				}

				if (isPawn(hSquarePos.y, hSquarePos.x) && eSquarePos == sf::Vector2u(it->first, it->second))
				{
					enPassant(hSquarePos, newSquarePos);
					enPassantPlayed = true;
				}

				setEnPassantSquare(hSquarePos, newSquarePos);
				updateCastlingStatus(hSquarePos, newSquarePos);

				// normal moves

				if (!enPassantPlayed && !castlingPlayed)
				{
					if (existsInVec(*it, promotionSquares))
						promotePawn(hSquarePos, newSquarePos);
					else
						makeMove(hSquarePos, newSquarePos);
				}

				saveMove(hSquarePos, newSquarePos);											// save move to list of playedMoves
				hSquarePos = getMouseSquare(mousePos, windowSize);							// hSquare set to destination square
				moveAllowed = true;															// set to true to highlight new square
				whiteToMove = !whiteToMove;													// change turns
				getAllThreats();															// find all threats for other player
				checkGameEnd();																// check for checkmate or stalemate
			}
		}

		movesVisible = false;																// stop displaying available moves after piece has been moved
	}
}


// Keyboard Input

void Board::flip()
{
	facingWhite = !facingWhite;

	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 8; ++j)
			std::swap(board[i][j], board[7 - i][7 - j]);			// swap the pieces

	hSquarePos = sf::Vector2u(7, 7) - hSquarePos;
	eSquarePos = sf::Vector2u(7, 7) - eSquarePos;

	for (size_t i = 0; i < availableMoves.size(); ++i)					// also change the list of moves if board is flipped
	{
		availableMoves[i].first = 7 - availableMoves[i].first;
		availableMoves[i].second = 7 - availableMoves[i].second;
	}
}

void Board::rgbBoardTheme()
{
	selectBoardTheme(boardThemes::rgb);
}

void Board::randomBoardTheme()
{
	selectBoardTheme(boardThemes::random);
}

void Board::randomPieceTheme()
{
	piecesTheme = pieceSets[randomSet];
}

void Board::togglePieceVisibilty()
{
	piecesVisible = !piecesVisible;
}

void Board::toggleLabelsVisibility()
{
	labelsVisible = !labelsVisible;
}

void Board::toggleThreatsVisibility()
{
	threatsVisible = !threatsVisible;
}

void Board::toggleNotationVisibility()
{
	notationVisible = !notationVisible;
}

void Board::toggleNotationAlignment()
{
	leftNotation = !leftNotation;
}


// ------------------------------------------- PRIVATE FUNCTIONS -------------------------------------------

// FEN

void Board::loadFen()
{
	std::istringstream iss(FEN);
	std::string str;

	getline(iss, str, ' ');
	loadPieces(str);

	getline(iss, str, ' ');
	loadActiveColor(str);

	getline(iss, str, ' ');
	loadCastlingRights(str);

	getline(iss, str, ' ');
	loadEnPassantTarget(str);

	getline(iss, str, ' ');
	halfMoves = intFromStr(str);

	getline(iss, str, ' ');
	fullMoves = intFromStr(str);
}

void Board::loadPieces(const std::string& str)
{
	int x = 0;		// square position on x axis
	int y = 0;		// square position on y axis (flipped in SFML)

	for (size_t i = 0; i < str.length(); ++i)
	{
		switch (str[i])
		{
		case '1':	case '2':	case '3':	case '4':	// empty spaces
		case '5':	case '6':	case '7':	case '8':
			for (int j = 0; j < str[i] - 48; ++j, ++x)	board[y][x] = 0;
			break;
		case 'K':	board[y][x] = 1;	White.kingPos = { x, y };	++x;	break;	// white King
		case 'Q':	board[y][x] = 2;								++x;	break;	// white Queen
		case 'R':	board[y][x] = 3;								++x;	break;	// white Rook
		case 'B':	board[y][x] = 4;								++x;	break;	// white Bishop
		case 'N':	board[y][x] = 5;								++x;	break;	// white kNight
		case 'P':	board[y][x] = 6;								++x;	break;	// white Pawn
		case 'k':	board[y][x] = -1;	Black.kingPos = { x, y };	++x;	break;	// black kING
		case 'q':	board[y][x] = -2;								++x;	break;	// black qUEEN
		case 'r':	board[y][x] = -3;								++x;	break;	// black rOOK
		case 'b':	board[y][x] = -4;								++x;	break;	// black bISHOP
		case 'n':	board[y][x] = -5;								++x;	break;	// black KnIGHT
		case 'p':	board[y][x] = -6;								++x;	break;	// black pAWN
		case '/':	x = 0;											++y;	break;	// next rank

		default:
			std::cerr << "Fatal Error! Invalid board position! Board::loadPosition()" << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}
}

void Board::loadActiveColor(const std::string& str)
{
	if (str == "w")
		whiteToMove = true;
	else if (str == "b")
		whiteToMove = false;
	else
	{
		std::cerr << "Fatal Error! Invalid active color! Board::loadActiveColor()" << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

void Board::loadCastlingRights(const std::string& str)
{
	if (str == "-")
		return;

	for (size_t i = 0; i < str.length(); ++i)
	{
		switch (str[i])
		{
		case 'K':
			White.kingsideCastling = true;
			break;
		case 'Q':
			White.queensideCastling = true;
			break;
		case 'k':
			Black.kingsideCastling = true;
			break;
		case 'q':
			Black.queensideCastling = true;
			break;

		default:
			std::cerr << "Fatal Error! Invalid castling rights! Board::loadCastlingRights()" << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}
}

void Board::loadEnPassantTarget(const std::string& str)
{
	if (str == "-" || str.length() == 2 && str[0] >= 'a' && str[0] <= 'h' && str[1] >= '1' && str[1] <= '8')
	{
		enPassantTarget = str;
	}
	else
	{
		std::cerr << "Fatal Error! Invalid En Passant target! Board::loadEnPassantTarget()" << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

int Board::intFromStr(const std::string& str)
{
	for (size_t i = 0; i < str.length(); ++i)
	{
		if (str[i] < '0' || str[i] > '9')
		{
			std::cerr << "Fatal Error! String does not contain integers! Board::intFromStr()" << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}

	return std::stoi(str);
}


// Themes

void Board::selectBoardTheme(const boardThemes& boardTheme)
{
	this->boardTheme = boardTheme;

	switch (boardTheme)
	{
	case boardThemes::blue:
		wColor = sf::Color(222, 227, 230, 255);
		bColor = sf::Color(140, 162, 173, 255);
		hColor = sf::Color(194, 215, 135, 255);
		break;
	case boardThemes::brown:
		wColor = sf::Color(240, 217, 181, 255);
		bColor = sf::Color(181, 136, 99, 255);
		hColor = sf::Color(205, 210, 106, 255);
		break;
	case boardThemes::green:
		wColor = sf::Color(238, 238, 210, 255);
		bColor = sf::Color(119, 149, 86, 255);
		hColor = sf::Color(79, 161, 142, 255);
		break;
	case boardThemes::purple:
		wColor = sf::Color(175, 175, 175, 255);
		bColor = sf::Color(167, 1, 166, 255);
		hColor = sf::Color(231, 162, 82, 255);
		break;
	case boardThemes::random:
	{
		hsv randomHSV = { randomFrac * 360, 1.0, 1.0 };
		rgb randomRGB = HSV2RGB(randomHSV);
		// set random values equal to values for rgb mode so it starts from here
		hsvColor = randomHSV;
		rgbColor = randomRGB;
		wColor = sf::Color(255, 255, 255, 255);
		bColor = sf::Color(randomRGB.r, randomRGB.g, randomRGB.b, 255);
		hColor = complementaryColor(bColor);
	}
	break;
	case boardThemes::rgb:
		rgbClock.restart();
		wColor = sf::Color(255, 255, 255, 255);
		bColor = sf::Color(rgbColor.r, rgbColor.g, rgbColor.b, 255);
		hColor = complementaryColor(bColor);
		break;

	default:
		std::cerr << "Fatal Error! Undefined board theme! Board::selectTheme()" << std::endl;
		std::exit(EXIT_FAILURE);
	}
}


// Colors

void Board::updateColors()
{
	if (rgbClock.getElapsedTime().asMilliseconds() > rgbTime / 10)
	{
		hsvColor.h += 0.1;

		if (hsvColor.h >= 360)
			hsvColor.h = 0;

		rgbColor = HSV2RGB(hsvColor);

		rgbClock.restart();

		bColor = sf::Color(rgbColor.r, rgbColor.g, rgbColor.b, 255);
		hColor = complementaryColor(bColor);
	}
}

hsv Board::RGB2HSV(const rgb& in)
{
	hsv out = { 0 };
	double r = in.r / 255.0;
	double g = in.g / 255.0;
	double b = in.b / 255.0;

	double min, max, delta;

	min = std::min({ r, g, b });
	max = std::max({ r, g, b });
	out.v = max;									// value
	delta = max - min;

	if (delta < 0.00001)
	{
		out.s = 0;
		out.h = 0;
		return out;
	}

	if (max > 0.0)
		out.s = (delta / max);						// saturation
	else
	{
		// r = g = b = 0
		out.s = 0.0;
		out.h = 0.0f;
		return out;
	}

	if (r == max)
		out.h = (g - b) / delta;					// between yellow and magenta

	if (g == max)
		out.h = 2.0 + (b - r) / delta;				// between cyan and yellow

	if (b == max)
		out.h = 4.0 + (r - g) / delta;				// between magenta and cyan

	out.h *= 60.0;									// degrees

	if (out.h < 0.0)
		out.h += 360.0;

	return out;
}

rgb Board::HSV2RGB(const hsv& in)
{
	rgb out = { 0 };
	double r, g, b;
	double hh, p, q, t, ff;
	long i;

	if (in.s <= 0.0)
	{
		out.r = unsigned int(in.v * 255);
		out.g = unsigned int(in.v * 255);
		out.b = unsigned int(in.v * 255);
		return out;
	}

	hh = in.h;
	if (hh >= 360.0) hh = 0.0;
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = in.v * (1.0 - in.s);
	q = in.v * (1.0 - (in.s * ff));
	t = in.v * (1.0 - (in.s * (1.0 - ff)));

	switch (i)
	{
	case 0:             r = in.v;       g = t;         b = p;           break;
	case 1:             r = q;          g = in.v;      b = p;           break;
	case 2:             r = p;          g = in.v;      b = t;           break;
	case 3:             r = p;          g = q;         b = in.v;        break;
	case 4:             r = t;          g = p;         b = in.v;        break;
	case 5: default:    r = in.v;       g = p;         b = q;           break;
	}

	out.r = unsigned int(r * 255);
	out.g = unsigned int(g * 255);
	out.b = unsigned int(b * 255);
	return out;
}

sf::Color Board::complementaryColor(const sf::Color& color)
{
	uint8_t max = std::max({ color.r, color.g, color.b });
	uint8_t min = std::min({ color.r, color.g, color.b });
	uint8_t r_ = max + min - color.r;
	uint8_t g_ = max + min - color.g;
	uint8_t b_ = max + min - color.b;
	return sf::Color(r_, g_, b_, color.a);
}


// Drawing

void Board::drawMoves(sf::RenderWindow& window)
{
	// color for outline
	rgb rtemp = { hColor.r, hColor.g, hColor.b };
	hsv htemp = RGB2HSV(rtemp);
	htemp.s = 0.5;
	htemp.v = 0.5;
	rtemp = HSV2RGB(htemp);

	sf::CircleShape circle(10.0f);
	circle.setFillColor(hColor);
	circle.setOutlineThickness(-2.0f);
	circle.setOutlineColor(sf::Color(rtemp.r, rtemp.g, rtemp.b, 175));

	sf::ConvexShape triangle(3);
	triangle.setFillColor(circle.getFillColor());
	triangle.setPoint(0, sf::Vector2f(0, 0));
	triangle.setPoint(1, sf::Vector2f(squareSize / 4, 0));
	triangle.setPoint(2, sf::Vector2f(0, squareSize / 4));
	triangle.setOutlineThickness(-2.0f);
	triangle.setOutlineColor(sf::Color(rtemp.r, rtemp.g, rtemp.b, 175));

	for (auto it = availableMoves.begin(); it != availableMoves.end(); ++it)
	{
		// Problem: Square appears when piece other than pawn attacks in en passant
		// Possible solution: get (i, j) as parameter and call isPieceCapture() function

		if (board[it->second][it->first] || eSquarePos == sf::Vector2u(it->first, it->second))		// if not empty square
		{
			triangle.setPosition(it->first * squareSize, it->second * squareSize);	// top left
			window.draw(triangle);

			triangle.rotate(90);
			triangle.move(squareSize, 0);		// top right
			window.draw(triangle);

			triangle.rotate(90);
			triangle.move(0, squareSize);		// bottom right
			window.draw(triangle);

			triangle.rotate(90);
			triangle.move(-squareSize, 0);		// bottom left
			window.draw(triangle);

			triangle.rotate(90);				// top left
		}
		else									// if empty square
		{
			circle.setPosition(it->first * squareSize, it->second * squareSize);
			circle.move((squareSize - circle.getLocalBounds().width) / 2, (squareSize - circle.getLocalBounds().height) / 2);
			window.draw(circle);
		}
	}
}

void Board::drawThreats(sf::RenderWindow& window)
{
	sf::CircleShape hexagon(32.0f, 6U);
	hexagon.setFillColor(sf::Color(hColor.r, hColor.g, hColor.b, 100));

	for (auto it = allThreats.begin(); it != allThreats.end(); ++it)
	{
		hexagon.setPosition(it->first * squareSize, it->second * squareSize);
		window.draw(hexagon);
	}
}

void Board::drawCheck(const int& i, const int& j, sf::RenderWindow& window)
{
	// Brainstorm -- checkered sphere texture ??
	sf::Texture checkTexture;
	checkTexture.setSmooth(true);

	if (!checkTexture.loadFromFile("../Resources/Textures/inverted_grey.png"))
	{
		std::cerr << "Fatal Error! Check texture not loaded! Board::drawCheck()" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	sf::RectangleShape check(sf::Vector2f(squareSize, squareSize));
	check.setFillColor(hColor);
	check.setTexture(&checkTexture);
	check.setPosition(j * squareSize, i * squareSize);
	window.draw(check);
}

void Board::drawPiece(const int& i, const int& j, sf::RenderWindow& window)
{
	std::string pieceType = "";

	switch (board[i][j])
	{
	case 0:							return;	// empty square
	case 1:		pieceType = "wK";	break;	// white King
	case 2:		pieceType = "wQ";	break;	// white Queen
	case 3:		pieceType = "wR";	break;	// white Rook
	case 4:		pieceType = "wB";	break;	// white Bishop
	case 5:		pieceType = "wN";	break;	// white kNight
	case 6:		pieceType = "wP";	break;	// white Pawn
	case -1:	pieceType = "bK";	break;	// black King
	case -2:	pieceType = "bQ";	break;	// black Queen
	case -3:	pieceType = "bR";	break;	// black Rook
	case -4:	pieceType = "bB";	break;	// black Bishop
	case -5:	pieceType = "bN";	break;	// black kNight
	case -6:	pieceType = "bP";	break;	// black Pawn

	default:
		std::cerr << "Fatal Error! Undefined piece type! Board::drawPiece()" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	std::string texturePath = "../Resources/Pieces/" + piecesTheme + "/" + pieceType + ".png";

	if (!piecesTexture.loadFromFile(texturePath))
	{
		std::cerr << "Fatal Error! Invalid texture path! Board::drawPiece()" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	sf::RectangleShape piece(sf::Vector2f(squareSize, squareSize));
	piece.setPosition(j * squareSize, i * squareSize);
	piece.setTexture(&piecesTexture);
	window.draw(piece);
}

void Board::drawLabels(const int& i, const int& j, sf::RenderWindow& window)
{
	sf::Text label;
	label.setFont(labelFont);
	label.setCharacterSize(15);
	label.setOutlineThickness(2.0f);
	label.setFillColor(wColor);
	label.setString(std::to_string(i) + ", " + std::to_string(j));
	label.setPosition(j * squareSize, i * squareSize);
	label.move((squareSize - label.getLocalBounds().width) / 2, (squareSize - label.getLocalBounds().height) / 2);
	window.draw(label);
}

void Board::drawNotation(const int& i, const int& j, sf::RenderWindow& window)
{
	if (i != 7 && j != 0 && j != 7)
		return;

	sf::Text notation("", notationFont, 16);
	notation.setFillColor((i + j) % 2 ? wColor : bColor);

	char a = (facingWhite ? j : 7 - j) + 97;	// convert to ASCII alphabet, consider board flip
	notation.setString(a);

	sf::FloatRect notationRect = notation.getLocalBounds();
	float xOffset = squareSize - notationRect.left - notationRect.width;
	float yOffset = squareSize - notationRect.top - notationRect.height;

	if (i == 7)
	{
		if (leftNotation)
			notation.setPosition(j * squareSize + xOffset, i * squareSize + yOffset);
		else
			notation.setPosition(j * squareSize, i * squareSize + yOffset);

		window.draw(notation);
	}

	char n = (facingWhite ? 7 - i : i) + 49;	// convert to ASCII number, consider board flip
	notation.setString(n);

	if (leftNotation && j == 0)
	{
		notation.setPosition(j * squareSize, i * squareSize);
		window.draw(notation);
	}

	if (!leftNotation && j == 7)
	{
		notation.setPosition(j * squareSize + xOffset, i * squareSize);
		window.draw(notation);
	}
}


// Squares

char Board::getRank(const int& i)
{
	return (facingWhite ? 7 - i : i) + 49;		// convert to ASCII number
}

char Board::getFile(const int& j)
{
	return (facingWhite ? j : 7 - j) + 97;		// convert to ASCII alphabet
}

Squares Board::squareFromStr(const std::string& str)
{
	static const std::unordered_map<std::string, Squares> table =
	{
		{"a1", Squares::a1}, {"a2", Squares::a2}, {"a3", Squares::a3}, {"a4", Squares::a4}, {"a5", Squares::a5}, {"a6", Squares::a6}, {"a7", Squares::a7}, {"a8", Squares::a8},
		{"b1", Squares::b1}, {"b2", Squares::b2}, {"b3", Squares::b3}, {"b4", Squares::b4}, {"b5", Squares::b5}, {"b6", Squares::b6}, {"b7", Squares::b7}, {"b8", Squares::b8},
		{"c1", Squares::c1}, {"c2", Squares::c2}, {"c3", Squares::c3}, {"c4", Squares::c4}, {"c5", Squares::c5}, {"c6", Squares::c6}, {"c7", Squares::c7}, {"c8", Squares::c8},
		{"d1", Squares::d1}, {"d2", Squares::d2}, {"d3", Squares::d3}, {"d4", Squares::d4}, {"d5", Squares::d5}, {"d6", Squares::d6}, {"d7", Squares::d7}, {"d8", Squares::d8},
		{"e1", Squares::e1}, {"e2", Squares::e2}, {"e3", Squares::e3}, {"e4", Squares::e4}, {"e5", Squares::e5}, {"e6", Squares::e6}, {"e7", Squares::e7}, {"e8", Squares::e8},
		{"f1", Squares::f1}, {"f2", Squares::f2}, {"f3", Squares::f3}, {"f4", Squares::f4}, {"f5", Squares::f5}, {"f6", Squares::f6}, {"f7", Squares::f7}, {"f8", Squares::f8},
		{"g1", Squares::g1}, {"g2", Squares::g2}, {"g3", Squares::g3}, {"g4", Squares::g4}, {"g5", Squares::g5}, {"g6", Squares::g6}, {"g7", Squares::g7}, {"g8", Squares::g8},
		{"h1", Squares::h1}, {"h2", Squares::h2}, {"h3", Squares::h3}, {"h4", Squares::h4}, {"h5", Squares::h5}, {"h6", Squares::h6}, {"h7", Squares::h7}, {"h8", Squares::h8},
	};

	auto it = table.find(str);

	if (it != table.end())
		return it->second;
	else
	{
		std::cerr << "Fatal Error! String does not correspond to a square! squareFromStr()" << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

Squares Board::squareFromPos(const sf::Vector2u& pos)
{
	std::string str = { getFile(pos.x), getRank(pos.y) };
	return squareFromStr(str);
}

int Board::getValueAt(const Squares& square)
{
	sf::Vector2u squarePos = getSquarePos(square);
	return board[squarePos.y][squarePos.x];
}

bool Board::isThreatened(const Squares& square)
{
	sf::Vector2u squarePos = getSquarePos(square);

	if (existsInVec(IntPair(squarePos.x, squarePos.y), allThreats))
		return true;
	else
		return false;
}

sf::Vector2u Board::getSquarePos(const Squares& square)
{
	sf::Vector2u squarePos = sf::Vector2u(int(square) % 8, int(square) / 8);

	if (!facingWhite)
		squarePos = sf::Vector2u(7, 7) - squarePos;

	return squarePos;
}


// Moves

sf::Vector2u Board::getMouseSquare(const sf::Vector2i& mousePos, const sf::Vector2u& windowSize)
{
	sf::Vector2u squarePos;																	// to store position of square on board
	int xOffset, yOffset, squareLength;														// offsets and square size that result due to resized window 

	xOffset = (windowSize.x > windowSize.y) ? (windowSize.x - windowSize.y) / 2 : 0;		// offset from left of window till board starts
	yOffset = (windowSize.y > windowSize.x) ? (windowSize.y - windowSize.x) / 2 : 0;		// offset from top of window till board starts
	squareLength = (windowSize.x > windowSize.y) ? windowSize.y / 8 : windowSize.x / 8;		// length of square on board in pixels

	squarePos.x = (mousePos.x - xOffset) / squareLength;									// convert mouse coordinates to square position on board
	squarePos.y = (mousePos.y - yOffset) / squareLength;

	return squarePos;
}

bool Board::existsInVec(const IntPair& val, const IntPairVec& vec)
{
	return std::find(vec.begin(), vec.end(), val) != vec.end();
}


// Pawn Functions

bool Board::isPawn(const int& i, const int& j)
{
	return abs(board[i][j]) == 6;
}

bool Board::pawnMovingUp(const int& i, const int& j)
{
	if (facingWhite && board[i][j] == 6 || !facingWhite && board[i][j] == -6)
		return true;
	else
		return false;
}

bool Board::pawnMovingDown(const int& i, const int& j)
{
	if (facingWhite && board[i][j] == -6 || !facingWhite && board[i][j] == 6)
		return true;
	else
		return false;
}


// Pawn Promotion

void Board::promotePawn(const sf::Vector2u& oldPos, const sf::Vector2u& newPos)
{
	int sign = board[oldPos.y][oldPos.x] / abs(board[oldPos.y][oldPos.x]);		// getting color of the pawn
	board[newPos.y][newPos.x] = sign * 2;										// make a new queen
	board[oldPos.y][oldPos.x] = 0;												// remove pawn from old position
}

void Board::setPromotionSquare(const sf::Vector2u& oldPos, const sf::Vector2u& newPos)
{
	if (oldPos.y == 1 && newPos.y == 0)							// pawn moves from second to first row
		promotionSquares.push_back({ newPos.x, newPos.y });

	if (oldPos.y == 6 && newPos.y == 7)							// pawn moves from seventh to eighth row
		promotionSquares.push_back({ newPos.x, newPos.y });
}


// En Passant

void Board::enPassant(const sf::Vector2u& oldPos, const sf::Vector2u& newPos)
{
	board[eSquarePos.y][eSquarePos.x] = board[oldPos.y][oldPos.x];		// add capturing pawn to en passant square
	board[oldPos.y][oldPos.x] = 0;										// remove capturing pawn from old position
	board[oldPos.y][newPos.x] = 0;										// remove the captured pawn
}

void Board::setEnPassantSquare(const sf::Vector2u& oldPos, const sf::Vector2u& newPos)
{
	eSquarePos = sf::Vector2u(8, 8);			// default dummy value overwritten if en passant possible, will never occur as out of board range

	if (oldPos.y == 6 && newPos.y == 4 && isPawn(oldPos.y, oldPos.x))
		eSquarePos = sf::Vector2u(oldPos.x, 5);

	if (oldPos.y == 1 && newPos.y == 3 && isPawn(oldPos.y, oldPos.x))
		eSquarePos = sf::Vector2u(oldPos.x, 2);
}

// Castling

void Board::castle(const sf::Vector2u& oldPos, const sf::Vector2u& newPos)
{
	// add functions that gauges whether:
	// any of the rooks have moved
	// any of the kings have moved
	// any of the rooks have been captured

	Squares oldSquare = squareFromPos(oldPos);
	Squares newSquare = squareFromPos(newPos);
	sf::Vector2u rookOldPos, rookNewPos;

	if (oldSquare == Squares::e1 && newSquare == Squares::g1)		// white kingside
	{
		rookOldPos = getSquarePos(Squares::h1);
		rookNewPos = getSquarePos(Squares::f1);
		White.kingsideCastling = false;
		White.queensideCastling = false;
	}

	if (oldSquare == Squares::e1 && newSquare == Squares::c1)		// white queenside
	{
		rookOldPos = getSquarePos(Squares::a1);
		rookNewPos = getSquarePos(Squares::d1);
		White.kingsideCastling = false;
		White.queensideCastling = false;
	}

	if (oldSquare == Squares::e8 && newSquare == Squares::g8)		// black kingside
	{
		rookOldPos = getSquarePos(Squares::h8);
		rookNewPos = getSquarePos(Squares::f8);
		Black.kingsideCastling = false;
		Black.queensideCastling = false;
	}

	if (oldSquare == Squares::e8 && newSquare == Squares::c8)		// black queenside
	{
		rookOldPos = getSquarePos(Squares::a8);
		rookNewPos = getSquarePos(Squares::d8);
		Black.kingsideCastling = false;
		Black.queensideCastling = false;
	}

	board[newPos.y][newPos.x] = board[oldPos.y][oldPos.x];
	board[oldPos.y][oldPos.x] = 0;
	board[rookNewPos.y][rookNewPos.x] = board[rookOldPos.y][rookOldPos.x];
	board[rookOldPos.y][rookOldPos.x] = 0;
}

bool Board::isKing(const int& i, const int& j)
{
	return abs(board[i][j]) == 1;
}

void Board::setCastlingSquares()
{
	castlingSquares.clear();
	sf::Vector2u castlingSquare;

	if (whiteToMove)
	{
		if (castlingPossible(White, true))					// white kingside castling
		{
			castlingSquare = getSquarePos(Squares::g1);
			castlingSquares.push_back(IntPair(castlingSquare.x, castlingSquare.y));
		}

		if (castlingPossible(White, false))					// white queenside castling
		{
			castlingSquare = getSquarePos(Squares::c1);
			castlingSquares.push_back(IntPair(castlingSquare.x, castlingSquare.y));
		}
	}

	if (!whiteToMove)
	{
		if (castlingPossible(Black, true))					// black kingside castling
		{
			castlingSquare = getSquarePos(Squares::g8);
			castlingSquares.push_back(IntPair(castlingSquare.x, castlingSquare.y));
		}

		if (castlingPossible(Black, false))					// black queenside castling
		{
			castlingSquare = getSquarePos(Squares::c8);
			castlingSquares.push_back(IntPair(castlingSquare.x, castlingSquare.y));
		}
	}
}

bool Board::castlingPossible(const Player& player, const bool& kingside)
{
	if (kingside)								// kingside castling
	{
		if (!player.kingsideCastling)			// if kingside castling rights are not available
			return false;

		Squares fSquare, gSquare;

		if (whiteToMove)	fSquare = Squares::f1, gSquare = Squares::g1;
		else				fSquare = Squares::f8, gSquare = Squares::g8;

		if (!player.inCheck && !player.kingMoved && !player.hRookMoved && !player.hRookCaptured &&
			!getValueAt(fSquare) && !getValueAt(gSquare) && !isThreatened(fSquare) && !isThreatened(gSquare))
			return true;

		return false;
	}
	else										// queenside castling
	{
		if (!player.queensideCastling)			// if queenside castling rights are not available
			return false;

		Squares bSquare, cSquare, dSquare;

		if (whiteToMove)	bSquare = Squares::b1, cSquare = Squares::c1, dSquare = Squares::d1;
		else				bSquare = Squares::b8, cSquare = Squares::c8, dSquare = Squares::d8;

		if (!player.inCheck && !player.kingMoved && !player.aRookMoved && !player.aRookCaptured &&
			!getValueAt(bSquare) && !getValueAt(cSquare) && !getValueAt(dSquare) &&
			!isThreatened(bSquare) && !isThreatened(cSquare) && !isThreatened(dSquare))
			return true;

		return false;
	}
}

void Board::updateCastlingStatus(const sf::Vector2u& oldPos, const sf::Vector2u& newPos)
{
	Squares oldSquare = squareFromPos(oldPos);
	Squares newSquare = squareFromPos(newPos);

	// piece moved

	switch (oldSquare)
	{
	case Squares::e1:						// white king moved
		White.kingsideCastling = false;
		White.queensideCastling = false;
		break;
	case Squares::a1:						// white 'a' rook moved
		White.queensideCastling = false;
		break;
	case Squares::h1:						// white 'h' rook moved
		White.kingsideCastling = false;
		break;
	case Squares::e8:						// black king moved
		Black.kingsideCastling = false;
		Black.queensideCastling = false;
		break;
	case Squares::a8:						// black 'a' rook moved
		Black.queensideCastling = false;
		break;
	case Squares::h8:						// black 'h' rook moved
		Black.kingsideCastling = false;
		break;
	default:
		break;
	}

	// piece captured

	switch (newSquare)
	{
	case Squares::a1:						// white 'a' rook captured
		White.queensideCastling = false;
		break;
	case Squares::h1:						// white 'h' rook captured
		White.kingsideCastling = false;
		break;
	case Squares::a8:						// black 'a' rook captured
		Black.queensideCastling = false;
		break;
	case Squares::h8:						// black 'h' rook captured
		Black.queensideCastling = false;
		break;
	default:
		break;
	}
}

// Move Handling

void Board::makeMove(const sf::Vector2u& oldPos, const sf::Vector2u& newPos)
{
	board[newPos.y][newPos.x] = board[oldPos.y][oldPos.x];			// capture piece
	board[oldPos.y][oldPos.x] = 0;									// remove piece from old position
}

void Board::saveMove(const sf::Vector2u& oldPos, const sf::Vector2u& newPos)
{
	std::string move{ getFile(oldPos.x), getRank(oldPos.y), getFile(newPos.x), getRank(newPos.y) };

	playedMoves.push_back(move);
	std::cout << "\n" << move << "\n";
}

void Board::undoMove()
{
	// Warning: This function does not work for en passant, castling etc.
	// Many attributes are not reset, only board position is reset.

	if (!playedMoves.empty())
	{
		playedMoves.pop_back();

		loadPosition();
	}
}

void Board::loadPosition()
{
	FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	loadFen();

	int index = 0;

	while (playedMoves.begin() + index < playedMoves.end())
	{
		std::string move = *(playedMoves.begin() + index);

		std::string oldStr = move.substr(0, 2);
		std::string newStr = move.substr(2, 2);

		makeMove(getSquarePos(squareFromStr(oldStr)), getSquarePos(squareFromStr(newStr)));
		whiteToMove = !whiteToMove;

		++index;
	}

	moveAllowed = false;
	getAllThreats();
}


// Threats Calculation

IntPairVec Board::findThreats(const int& i, const int& j, const IntPairVec& delta, const bool& longRange)
{
	IntPairVec threats;

	for (auto it = delta.begin(); it != delta.end(); ++it)									// loop over all the directions that piece can move in
	{
		int dx = it->first;																	// change in direction in x axis
		int dy = it->second;																// change in direction in y axis (flipped in SFML)

		do
		{
			if (!isOnBoard(j + dx, i + dy))													// if out of board
				break;

			threats.push_back({ j + dx, i + dy });											// store move as (x, y) coordinates

			if (board[i + dy][j + dx])														// if square is occupied by piece
				break;

			dx = dx ? dx + dx / abs(dx) : 0;												// signed increment with divide by zero check
			dy = dy ? dy + dy / abs(dy) : 0;

		} while (longRange);																// continue searching along a line or diagonal
	}

	return threats;
}

IntPairVec Board::getPawnThreats(const int& i, const int& j)
{
	IntPairVec delta;

	if (pawnMovingUp(i, j))																// pawns moving upwards
	{
		delta.push_back({ -1, -1 });													// capturing on left diagonal going up
		delta.push_back({ 1, -1 });														// capturing on right diagonal going up
	}

	if (pawnMovingDown(i, j))															// pawns moving downwards
	{
		delta.push_back({ -1, 1 });														// capturing on left diagonal coming down
		delta.push_back({ 1, 1 });														// capturing on right diagonal coming down
	}

	return findThreats(i, j, delta, false);										// find the squares threatened by piece
}

IntPairVec Board::getKnightThreats(const int& i, const int& j)
{
	IntPairVec delta = { {1, -2}, {2, -1}, {2, 1}, {1, 2}, {-1, 2}, {-2, 1}, {-2, -1}, {-1, -2} };
	return findThreats(i, j, delta, false);
}

IntPairVec Board::getBishopThreats(const int& i, const int& j)
{
	IntPairVec delta = { {1, -1}, {1, 1}, {-1, 1}, {-1, -1} };
	return findThreats(i, j, delta, true);
}

IntPairVec Board::getRookThreats(const int& i, const int& j)
{
	IntPairVec delta = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} };
	return findThreats(i, j, delta, true);
}

IntPairVec Board::getQueenThreats(const int& i, const int& j)
{
	IntPairVec delta = { {0, -1}, {1, 0}, {0, 1}, {-1, 0}, {1, -1}, {1, 1}, {-1, 1}, {-1, -1} };
	return findThreats(i, j, delta, true);
}

IntPairVec Board::getKingThreats(const int& i, const int& j)
{
	IntPairVec delta = { {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1} };
	return findThreats(i, j, delta, false);
}

IntPairVec Board::getPieceThreats(const int& i, const int& j)
{
	IntPairVec threats;

	switch (board[i][j])
	{
	case 0:														break;		// empty square
	case 1:	case -1:	threats = getKingThreats(i, j);			break;		// King
	case 2:	case -2:	threats = getQueenThreats(i, j);		break;		// Queen
	case 3:	case -3:	threats = getRookThreats(i, j);			break;		// Rook
	case 4:	case -4:	threats = getBishopThreats(i, j);		break;		// Bishop
	case 5:	case -5:	threats = getKnightThreats(i, j);		break;		// Knight
	case 6:	case -6:	threats = getPawnThreats(i, j);			break;		// Pawn
	default:
		std::cerr << "Fatal Error! Undefined piece type! Board::getPieceThreats()" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	for (auto it = threats.begin(); it != threats.end(); ++it)
	{
		if (!existsInVec(*it, allThreats))									// if current threat does not exist in allThreats (prevent duplication)
			allThreats.push_back(*it);										// add to threats
	}

	return threats;
}


// Moves Calculation

IntPairVec Board::getPawnPushes(const int& i, const int& j)
{
	IntPairVec pushes;

	// only consider push if the destination square is empty
	if (pawnMovingUp(i, j))																	// pawns moving upwards
	{
		if (!board[i - 1][j])																// if empty square in front of pawn
		{
			pushes.push_back({ j, i - 1 });													// push one square

			if (i == 6 && !board[i - 2][j])													// if first move and empty square in front, push two squares
				pushes.push_back({ j, i - 2 });
		}
	}

	if (pawnMovingDown(i, j))																// pawns moving downwards
	{
		if (!board[i + 1][j])																// if empty square in front of pawn
		{
			pushes.push_back({ j, i + 1 });													// push one square

			if (i == 1 && !board[i + 2][j])													// if first move and empty square in front, push two squares
				pushes.push_back({ j, i + 2 });
		}
	}

	return pushes;
}

IntPairVec Board::getPawnMoves(const int& i, const int& j)
{
	IntPairVec threats = getPawnThreats(i, j);
	IntPairVec wrongThreats, moves;

	// threats are considered moves only if opponent piece on pawn diagonal or enPassantSquare
	for (size_t k = 0; k < threats.size(); ++k)
	{
		if (!isPieceCapture(i, j, threats[k].second, threats[k].first))
			wrongThreats.push_back(threats[k]);
	}

	for (IntPair& t : threats)
	{
		if (!existsInVec(t, wrongThreats))
			moves.push_back(t);
	}

	IntPairVec pushes = getPawnPushes(i, j);

	moves.insert(moves.end(), pushes.begin(), pushes.end());
	return moves;
}

IntPairVec Board::getPieceMoves(const int& i, const int& j)
{
	IntPairVec moves, wrongMoves, correctMoves;		// moves stores candidate moves, wrongMoves stores illegal moves

	if (isPawn(i, j))
		moves = getPawnMoves(i, j);					// pawns move and capture differently
	else
		moves = getPieceThreats(i, j);				// other pieces move and capture in same manner

	// add illegal moves to wrongMoves
	for (size_t k = 0; k < moves.size(); ++k)
	{
		if (!isLegalMove(i, j, moves[k].second, moves[k].first))
			wrongMoves.push_back(moves[k]);
	}

	// add those moves to availableMoves which do not exist in wrongMoves
	for (IntPair& m : moves)
	{
		if (!existsInVec(m, wrongMoves))
			correctMoves.push_back(m);
	}

	// populate list of squares for pawn promotion
	promotionSquares.clear();

	if (isPawn(i, j))
	{
		for (IntPair& move : correctMoves)
			setPromotionSquare(sf::Vector2u(j, i), sf::Vector2u(move.first, move.second));
	}

	// populate list of squares for castling

	if (isKing(i, j))
	{
		setCastlingSquares();
		correctMoves.insert(correctMoves.end(), castlingSquares.begin(), castlingSquares.end());
	}

	return correctMoves;
}


// Move Validation

bool Board::isOnBoard(const int& x, const int& y)
{
	if (x < 0 || x > 7 || y < 0 || y > 7)	return false;
	else									return true;
}

bool Board::isPieceCapture(const int& i, const int& j, const int& new_i, const int& new_j)
{
	if (isPawn(i, j) && eSquarePos == sf::Vector2u(new_j, new_i))		// en passant capture for pawns
		return true;

	if (board[i][j] > 0 && board[new_i][new_j] < 0)				// white can capture black piece
		return true;

	if (board[i][j] < 0 && board[new_i][new_j] > 0)				// black and capture white piece
		return true;

	return false;
}

bool Board::putsInCheck(const int& i, const int& j, const int& new_i, const int& new_j)
{
	// store board position

	int tempBoard[8][8] = { 0 };

	for (int r = 0; r < 8; ++r)
		for (int s = 0; s < 8; ++s)
			tempBoard[r][s] = board[r][s];

	// "make" the  move

	if (isPawn(i, j) && eSquarePos == sf::Vector2u(new_j, new_i))
		enPassant(sf::Vector2u(j, i), sf::Vector2u(new_j, new_i));
	else
		makeMove(sf::Vector2u(j, i), sf::Vector2u(new_j, new_i));

	// find new threats and see if king ends up in check

	getAllThreats();

	bool putInCheck = whiteToMove && White.inCheck || !whiteToMove && Black.inCheck;

	// restore the position of the board

	for (int r = 0; r < 8; ++r)
		for (int s = 0; s < 8; ++s)
			board[r][s] = tempBoard[r][s];

	// get original threats and player inCheck flags

	getAllThreats();

	if (putInCheck)		return true;
	else				return false;
}

bool Board::isLegalMove(const int& i, const int& j, const int& new_i, const int& new_j)
{
	// this function will be applied to check candidate moves calculated by findThreats function
	// assuming move to be checked is within board bounds

	if (!board[new_i][new_j] || isPieceCapture(i, j, new_i, new_j))				// destination square is empty OR opponent piece captured
	{
		if (isKing(new_i, new_j))										// king capture is not allowed
			return false;

		if (putsInCheck(i, j, new_i, new_j))									// if move puts king in check (OR does not move king out of check)
			return false;

		return true;															// passes all tests
	}

	return false;			// destination piece is player's piece
}


// Checks

void Board::isCheck()
{
	// find position of white and black kings

	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 8; ++j)
		{
			if (board[i][j] == 1)		White.kingPos = { j, i };
			if (board[i][j] == -1)		Black.kingPos = { j, i };
		}
	}

	White.inCheck = false;
	Black.inCheck = false;

	for (auto it = allThreats.begin(); it != allThreats.end(); ++it)
	{
		if (whiteToMove && White.kingPos == *it)
			White.inCheck = true;

		if (!whiteToMove && Black.kingPos == *it)
			Black.inCheck = true;
	}
}

void Board::getAllThreats()
{
	allThreats.clear();

	for (int i = 0; i < 8; ++i)
		for (int j = 0; j < 8; ++j)
			if (whiteToMove && board[i][j] < 0 || !whiteToMove && board[i][j] > 0)
				getPieceThreats(i, j);

	isCheck();
}

void Board::checkGameEnd()
{
	IntPairVec moves, allMoves;

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if (whiteToMove && board[i][j] > 0 || !whiteToMove && board[i][j] < 0)
			{
				moves = getPieceMoves(i, j);
				allMoves.insert(allMoves.end(), moves.begin(), moves.end());

				if (!allMoves.empty())	return;				// if any possible moves, return to save time
			}
		}
	}

	if (allMoves.empty() && White.inCheck || Black.inCheck)
	{
		checkmate = true;
		std::cout << "\nCheckmate! " << (whiteToMove ? "Black" : "White") << " wins the game." << std::endl;
		bColor = sf::Color(25, 25, 25, 255);
	}

	if (allMoves.empty() && !White.inCheck && !Black.inCheck)
	{
		stalemate = true;
		std::cout << "\nStalemate! Game ends in a draw." << std::endl;
		bColor = sf::Color(25, 25, 25, 255);
	}
}