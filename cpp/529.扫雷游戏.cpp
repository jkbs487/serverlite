/*
 * @lc app=leetcode.cn id=529 lang=cpp
 *
 * [529] 扫雷游戏
 */

// @lc code=start
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
};
// @lc code=end

