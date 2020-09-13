/*
 * @lc app=leetcode.cn id=39 lang=cpp
 *
 * [39] 组合总和
 */

// @lc code=start
class Solution {
public:
    vector<vector<int>> combinationSum(vector<int>& candidates, int target) {
        vector<vector<int>> ans;
        vector<int> temp;
        dfs(ans, candidates, temp, target, 0, 0);
        return ans;
    }

    void dfs(vector<vector<int>>& ans, vector<int> candidates, vector<int> temp, int target, int index, int sum) {
        if(sum == target) {
            ans.push_back(temp);
            return;
        }
        if(sum > target) return;
        for(int i = index; i < candidates.size(); i++) {
            if(candidates[i] > target) continue;
            temp.push_back(candidates[i]);
            dfs(ans, candidates, temp, target, i, sum+candidates[i]);
            temp.pop_back();
        }
        return;
    }
};
// @lc code=end

