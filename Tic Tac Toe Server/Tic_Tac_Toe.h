#pragma once

class TicTacToe {
public:
	char board[9]{ '0','1','2','3','4','5','6','7','8' };
	char player[2] = { 'X', 'O' };
	bool turn = false;
	void makeMove(int);
	int sendBoard(char* );
	int status();
};
