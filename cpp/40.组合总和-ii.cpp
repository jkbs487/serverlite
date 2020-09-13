/*
 * @lc app=leetcode.cn id=40 lang=cpp
 *
 * [40] 组合总和 II
 */

// @lc code=start
class Solution {
public:
    vector<vector<int>> combinationSum2(vector<int>& candidates, int target) {
        vector<vector<int>> ans;
        sort(candidates.begin(), candidates.end());
        dfs(ans, candidates, {}, target, 0);
        return ans;
    }

    void dfs(vector<vector<int>>& ans, vector<int>& candidates, vector<int> temp, int target, int index) {
        if(target == 0) {
            ans.push_back(temp);
            return;
        }
        for(int i = index; i < candidates.size() && target - candidates[i] >= 0; ++i) {
            if(i > index && candidates[i] == candidates[i-1])
                continue;
            temp.push_back(candidates[i]);
            dfs(ans, candidates, temp, target-candidates[i], i+1);
            res.pop_back();
        }
        return;
    }
};
// @lc code=end

