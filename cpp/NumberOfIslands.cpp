// Source : https://leetcode-cn.com/problems/number-of-islands/
// Author : jkbs487
// Date   : 2021-05-30

/***************************************************************************************************** 
 *
 * Given an m x n 2D binary grid grid which represents a map of '1's (land) and '0's (water), return 
 * the number of islands.
 * 
 * An island is surrounded by water and is formed by connecting adjacent lands horizontally or 
 * vertically. You may assume all four edges of the grid are all surrounded by water.
 * 
 * Example 1:
 * 
 * Input: grid = [
 *   ["1","1","1","1","0"],
 *   ["1","1","0","1","0"],
 *   ["1","1","0","0","0"],
 *   ["0","0","0","0","0"]
 * ]
 * Output: 1
 * 
 * Example 2:
 * 
 * Input: grid = [
 *   ["1","1","0","0","0"],
 *   ["1","1","0","0","0"],
 *   ["0","0","1","0","0"],
 *   ["0","0","0","1","1"]
 * ]
 * Output: 3
 * 
 * Constraints:
 * 
 * 	m == grid.length
 * 	n == grid[i].length
 * 	1 <= m, n <= 300
 * 	grid[i][j] is '0' or '1'.
 ******************************************************************************************************/

//DFS
// "1": island
// "0": water
class Solution {
public:
    int numIslands(vector<vector<char>>& grid) {
        int ans = 0;
        int m = grid.size();
        int n = grid[0].size();
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                if (grid[i][j] == '1') {
                    ans++;
                    dfs(grid, i, j, m, n);
                }
            }
        }
        return ans;
    }

    void dfs(vector<vector<char>>& grid, int i, int j, int m, int n) {
        if (grid[i][j] != '1') return;
        grid[i][j] = '0';
        if (i + 1 < m)
            dfs(grid, i+1, j, m, n);
        if (i - 1 >= 0)
            dfs(grid, i-1, j, m, n);
        if (j + 1 < n) 
            dfs(grid, i, j+1, m, n);
        if (j - 1 >= 0) 
            dfs(grid, i, j-1, m, n);
        return;
    }
};
