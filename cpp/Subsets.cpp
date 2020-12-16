// Source : https://leetcode-cn.com/problems/subsets/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Given an integer array nums, return all possible subsets (the power set).
 * 
 * The solution set must not contain duplicate subsets.
 * 
 * Example 1:
 * 
 * Input: nums = [1,2,3]
 * Output: [[],[1],[2],[1,2],[3],[1,3],[2,3],[1,2,3]]
 * 
 * Example 2:
 * 
 * Input: nums = [0]
 * Output: [[],[0]]
 * 
 * Constraints:
 * 
 * 	1 <= nums.length <= 10
 * 	-10 <= nums[i] <= 10
 ******************************************************************************************************/

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
