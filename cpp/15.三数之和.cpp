/*
 * @lc app=leetcode.cn id=15 lang=cpp
 *
 * [15] 三数之和
 */

// @lc code=start
class Solution {
public:
    vector<vector<int>> threeSum(vector<int>& nums) {
        vector<vector<int>> ans;
        if(nums.empty()) return ans;
        sort(nums.begin(), nums.end());
        int len = nums.size();
        for(int i = 0; i < len; i++) {
            int left = i+1, right = len-1;
            while(left < right) {
                int sum = nums.at(left) + nums.at(right) + nums.at(i);
                if(sum < 0) left++;
                else if(sum > 0) right--;
                else {
                    ans.push_back({nums.at(i), nums.at(left), nums.at(right)});
                    while(left+1 < len && nums.at(left) == nums.at(left+1)) left++;
                    while(right-1 >= 0 && nums.at(right) == nums.at(right-1)) right--;
                    left++;
                    right--;
                }
            }
            while(i+1 < len && nums.at(i+1) == nums.at(i)) i++;
        }
        return ans;
    }
};
// @lc code=end

