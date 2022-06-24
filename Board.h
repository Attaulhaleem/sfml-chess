#pragma once
#include "SFML/Graphics.hpp"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#define randomFrac (double)rand() / RAND_MAX
#define randomSet rand() % 24

typedef std::pair<int, int> IntPair;
typedef std::vector<IntPair> IntPairVec;
typedef std::vector<std::string> StrVec;

struct rgb
{
	uint32_t r;			// 0 - 255
	uint32_t g;			// 0 - 255
	uint32_t b;			// 0 - 255
};

struct hsv
{
	double h;			// 0 - 360
	double s;			// 0 - 1
	double v;			// 0 - 1
};

enum class boardThemes { blue, brown, green, purple, random, rgb };

enum class Squares
{
	a8, b8, c8, d8, e8, f8, g8, h8,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a1, b1, c1, d1, e1, f1, g1, h1,
};

struct Player
{
	IntPair kingPos;			// location of the king on board (0-7, 0-7)
	bool inCheck;				// true if king is in check
	bool kingMoved;				// true if king has been moved at least once
	bool aRookMoved;			// true if rook on 'a' file has been moved at least once
	bool hRookMoved;			// true if rook on 'h' file has been moved at least once
	bool aRookCaptured;			// true if rook on 'a' file has been captured
	bool hRookCaptured;			// true if rook on 'h' file has been captured
	bool kingsideCastling;		// true if player reserves right to castle kingside
	bool queensideCastling;		// true if player reserves right to castle queenside

	Player()
	{
		kingPos = { 0, 0 };		// default value set by loadFen() in Board() anyway
		inCheck = false;
		kingMoved = false;
		aRookMoved = false;
		hRookMoved = false;
		aRookCaptured = false;
		hRookCaptured = false;
		kingsideCastling = false;
		queensideCastling = false;
	}
};

class Board
{
	// Public Functions
public:
	// Constructor

	Board(const boardThemes& boardTheme, const std::string& piecesTheme);

	// Drawing

	void draw(sf::RenderWindow& window);

	// Mouse Input

	void resize(sf::View& view, const float& viewLength, const sf::Vector2u& windowSize);
	void movePiece(const sf::Vector2i& mousePos, const sf::Vector2u& windowSize, const bool& isMousePressed);

	// Keyboard Input

	void flip();
	void undoMove();
	void rgbBoardTheme();
	void randomBoardTheme();
	void randomPieceTheme();
	void togglePieceVisibilty();
	void toggleLabelsVisibility();
	void toggleThreatsVisibility();
	void toggleNotationVisibility();
	void toggleNotationAlignment();

	// Private Functions
private:
	// FEN

	void loadFen();
	void loadPieces(const std::string& str);
	void loadActiveColor(const std::string& str);
	void loadCastlingRights(const std::string& str);
	void loadEnPassantTarget(const std::string& str);
	int  intFromStr(const std::string& str);

	// Themes

	void selectBoardTheme(const boardThemes& boardTheme);

	// Colors

	void updateColors();
	hsv RGB2HSV(const rgb& in);
	rgb HSV2RGB(const hsv& in);
	sf::Color complementaryColor(const sf::Color& color);

	// Drawing

	void drawMoves(sf::RenderWindow& window);
	void drawThreats(sf::RenderWindow& window);
	void drawCheck(const int& i, const int& j, sf::RenderWindow& window);
	void drawPiece(const int& i, const int& j, sf::RenderWindow& window);
	void drawLabels(const int& i, const int& j, sf::RenderWindow& window);
	void drawNotation(const int& i, const int& j, sf::RenderWindow& window);

	// Squares

	char getRank(const int& i);
	char getFile(const int& j);

	Squares squareFromStr(const std::string& str);
	Squares squareFromPos(const sf::Vector2u& pos);

	int getValueAt(const Squares& square);
	bool isThreatened(const Squares& square);
	sf::Vector2u getSquarePos(const Squares& square);

	// Moves

	sf::Vector2u getMouseSquare(const sf::Vector2i& mousePos, const sf::Vector2u& windowSize);

	bool existsInVec(const IntPair& val, const IntPairVec& vec);

	// Pawns

	bool isPawn(const int& i, const int& j);
	bool pawnMovingUp(const int& i, const int& j);
	bool pawnMovingDown(const int& i, const int& j);

	// Pawn Promotion

	void promotePawn(const sf::Vector2u& oldPos, const sf::Vector2u& newPos);
	void setPromotionSquare(const sf::Vector2u& oldPos, const sf::Vector2u& newPos);

	// En Passant

	void enPassant(const sf::Vector2u& oldPos, const sf::Vector2u& newPos);
	void setEnPassantSquare(const sf::Vector2u& oldPos, const sf::Vector2u& newPos);

	// Castling

	void setCastlingSquares();

	bool isKing(const int& i, const int& j);
	bool castlingPossible(const Player& player, const bool& kingside);

	void castle(const sf::Vector2u& oldPos, const sf::Vector2u& newPos);
	void updateCastlingStatus(const sf::Vector2u& oldPos, const sf::Vector2u& newPos);

	// Move Handling

	void makeMove(const sf::Vector2u& oldPos, const sf::Vector2u& newPos);
	void saveMove(const sf::Vector2u& oldPos, const sf::Vector2u& newPos);
	void loadPosition();

	// Threats Calculation

	IntPairVec findThreats(const int& i, const int& j, const IntPairVec& delta, const bool& longRange);

	IntPairVec getPawnThreats(const int& i, const int& j);
	IntPairVec getKnightThreats(const int& i, const int& j);
	IntPairVec getBishopThreats(const int& i, const int& j);
	IntPairVec getRookThreats(const int& i, const int& j);
	IntPairVec getQueenThreats(const int& i, const int& j);
	IntPairVec getKingThreats(const int& i, const int& j);
	IntPairVec getPieceThreats(const int& i, const int& j);

	// Moves Calculation

	IntPairVec getPawnPushes(const int& i, const int& j);
	IntPairVec getPawnMoves(const int& i, const int& j);
	IntPairVec getPieceMoves(const int& i, const int& j);

	// Move Validation

	bool isOnBoard(const int& x, const int& y);
	bool isPieceCapture(const int& i, const int& j, const int& new_i, const int& new_j);
	bool putsInCheck(const int& i, const int& j, const int& new_i, const int& new_j);
	bool isLegalMove(const int& i, const int& j, const int& new_i, const int& new_j);

	// Checks

	void isCheck();
	void getAllThreats();
	void checkGameEnd();

	// Private Variables
private:
	Player White, Black;			// two players

	hsv hsvColor;					// HSV color of 'black' squares - RGB mode
	rgb rgbColor;					// RGB color of 'black' squares - RGB mode
	sf::Clock rgbClock;				// keeps track of time for changing colors - RGB mode
	int rgbTime;					// time (in ms) after which hue of color changes by one degree - RGB mode

	float squareSize;				// size (in pixels) of a board square as shown on screen initially

	bool whiteToMove;				// true if white's move, false if black's move
	bool facingWhite;				// true if the board is from white's perspective, false if flipped towards black
	bool moveAllowed;				// true if moving the selected piece is allowed

	boardThemes boardTheme;			// defines color theme for the board

	sf::Color wColor;				// defines color for white squares
	sf::Color bColor;				// defines color for black squares
	sf::Color hColor;				// defines color for highlighted (currently selected) square

	sf::Vector2u hSquarePos;		// (j, i) position of highlighted square on board
	sf::Vector2u eSquarePos;		// (j, i) position of En Passant square on board

	sf::Texture squareTexture;		// texture for board squares

	bool piecesVisible;				// pieces visible if set to true
	sf::Texture piecesTexture;		// texture used for pieces
	std::string piecesTheme;		// current theme being used for pieces

	bool movesVisible;				// draw legal moves for selected piece if set to true
	bool threatsVisible;			// draw threats by the opponent if set to true

	bool notationVisible;			// algebraic notation visible if set to true
	bool leftNotation;				// numbers drawn in left file if true and right file if false, also slightly affects alphabet placement
	sf::Font notationFont;			// font used for drawing algebraic notation

	bool labelsVisible;				// (i, j) of each square visible if set to true
	sf::Font labelFont;				// font used for drawing labels

	int board[8][8];				// (0)Empty (1)King (2)Queen (3)Rook (4)Bishop (5)Knight (6)Pawn | pos for white, neg for black

	StrVec playedMoves;				// list of moves played in the game (coordinate notation) e.g. e2e4

	IntPairVec allThreats;			// list of squares threated by the enemy
	IntPairVec availableMoves;		// list of moves available to the current piece selected
	IntPairVec promotionSquares;	// list of squares where pawn promotion can take place
	IntPairVec castlingSquares;		// list of squares where player's king can safely castle to (max 2)

	std::string FEN;				// stores FEN position

	bool whiteKingMoved;			// true if white king has been moved at least once
	bool blackKingMoved;			// true if black king has been moved at least once
	bool a1RookMoved;				// true if rook on a1 has been moved at least once
	bool h1RookMoved;				// true if rook on h1 has been moved at least once
	bool a8RookMoved;				// true if rook on a8 has been moved at least once
	bool h8RookMoved;				// true if rook on h8 has been moved at least once

	std::string enPassantTarget;	// stores target square if en passant is possible, otherwise stores "-"

	int halfMoves;					// number of half moves
	int fullMoves;					// number of full moves

	bool checkmate;					// becomes true if game ends in checkmate
	bool stalemate;					// becomes true if game ends in stalemate

	const char* pieceSets[24] = { "alpha", "california", "cardinal", "cburnett", "chess7", "chessnut",
		"companion", "fantasy", "fresca", "gioco", "governor", "horsey", "icpieces", "kosal", "leipzig",
		"libra", "maestro", "merida", "pirouetti", "pixel", "riohacha", "spatial", "staunty", "tatiana" };
};