// Source : https://leetcode-cn.com/problems/minesweeper/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Let's play the minesweeper game (Wikipedia, online game)!
 * 
 * You are given a 2D char matrix representing the game board. 'M' represents an unrevealed mine, 'E' 
 * represents an unrevealed empty square, 'B' represents a revealed blank square that has no adjacent 
 * (above, below, left, right, and all 4 diagonals) mines, digit ('1' to '8') represents how many 
 * mines are adjacent to this revealed square, and finally 'X' represents a revealed mine.
 * 
 * Now given the next click position (row and column indices) among all the unrevealed squares ('M' or 
 * 'E'), return the board after revealing this position according to the following rules:
 * 
 * 	If a mine ('M') is revealed, then the game is over - change it to 'X'.
 * 	If an empty square ('E') with no adjacent mines is revealed, then change it to revealed 
 * blank ('B') and all of its adjacent unrevealed squares should be revealed recursively.
 * 	If an empty square ('E') with at least one adjacent mine is revealed, then change it to a 
 * digit ('1' to '8') representing the number of adjacent mines.
 * 	Return the board when no more squares will be revealed.
 * 
 * Example 1:
 * 
 * Input: 
 * 
 * [['E', 'E', 'E', 'E', 'E'],
 *  ['E', 'E', 'M', 'E', 'E'],
 *  ['E', 'E', 'E', 'E', 'E'],
 *  ['E', 'E', 'E', 'E', 'E']]
 * 
 * Click : [3,0]
 * 
 * Output: 
 * 
 * [['B', '1', 'E', '1', 'B'],
 *  ['B', '1', 'M', '1', 'B'],
 *  ['B', '1', '1', '1', 'B'],
 *  ['B', 'B', 'B', 'B', 'B']]
 * 
 * Explanation:
 * 
 * Example 2:
 * 
 * Input: 
 * 
 * [['B', '1', 'E', '1', 'B'],
 *  ['B', '1', 'M', '1', 'B'],
 *  ['B', '1', '1', '1', 'B'],
 *  ['B', 'B', 'B', 'B', 'B']]
 * 
 * Click : [1,2]
 * 
 * Output: 
 * 
 * [['B', '1', 'E', '1', 'B'],
 *  ['B', '1', 'X', '1', 'B'],
 *  ['B', '1', '1', '1', 'B'],
 *  ['B', 'B', 'B', 'B', 'B']]
 * 
 * Explanation:
 * 
 * Note:
 * 
 * 	The range of the input matrix's height and width is [1,50].
 * 	The click position will only be an unrevealed square ('M' or 'E'), which also means the 
 * input board contains at least one clickable square.
 * 	The input board won't be a stage when game is over (some mines have been revealed).
 * 	For simplicity, not mentioned rules should be ignored in this problem. For example, you 
 * don't need to reveal all the unrevealed mines when the game is over, consider any cases that you 
 * will win the game or flag any squares.
 * 
 ******************************************************************************************************/

class Solution {
public:
    vector<pair<int, int>> direction = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    vector<vector<char>> updateBoard(vector<vector<char>>& board, vector<int>& click) {
        vector<vector<int>> visit(board.size(), vector<int>(board[0].size()));
        int x = click[0], y = click[1]; 
        if(board[x][y] == 'M') {
            board[x][y] = 'X';
            return board;
        }
        dfs(board, visit, x, y);
        return board;
    }

    void dfs(vector<vector<char>>& board, vector<vector<int>>& visit, int x, int y) {
        if(x >= board.size() || x < 0 || y >= board[0].size() || y < 0 || board[x][y] != 'E') 
            return;
        int mines = computeMines(board, x, y);
        if(mines > 0) board[x][y] = mines + '0';
        else board[x][y] = 'B';
        visit[x][y] = 1;
        if(board[x][y] == 'B') {
            for(pair<int, int> dir : direction) {
                int posX = x + dir.first;
                int posY = y + dir.second;
                if(posX >= 0 && posX < board.size() && posY >= 0 && posY < board[0].size() && !visit[posX][posY]) 
                    dfs(board, visit, posX, posY);
            }
        }
        return;
    }
    
    int computeMines(vector<vector<char>>& board, int x, int y) {
        int mines = 0;
        for(pair<int, int> dir : direction) {
            int posX = x + dir.first;
            int posY = y + dir.second;
            if(posX >= 0 && posX < board.size() && posY >= 0 && posY < board[0].size() && board[posX][posY] == 'M') 
                mines++;
        }
        return mines;
    }
};
