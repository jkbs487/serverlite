/*
 * @lc app=leetcode.cn id=78 lang=cpp
 *
 * [78] 子集
 */

// @lc code=start
class Solution {
public:
    vector<vector<int>> subsets(vector<int>& nums) {
        vector<vector<int>> ans;
        dfs(ans, {}, nums, 0);
        return ans;
    }
    void dfs(vector<vector<int>>& ans, vector<int> temp, vector<int> nums, int index) {
        ans.push_back(temp);
        for(int i = index; i < nums.size(); i++){
            temp.push_back(nums[i]);
            dfs(ans, temp, nums, i+1);
            temp.pop_back();
        }
        return;
    }
};
// @lc code=end

