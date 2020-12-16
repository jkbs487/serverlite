// Source : https://leetcode-cn.com/problems/3sum/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Given an array nums of n integers, are there elements a, b, c in nums such that a + b + c = 0? Find 
 * all unique triplets in the array which gives the sum of zero.
 * 
 * Notice that the solution set must not contain duplicate triplets.
 * 
 * Example 1:
 * Input: nums = [-1,0,1,2,-1,-4]
 * Output: [[-1,-1,2],[-1,0,1]]
 * Example 2:
 * Input: nums = []
 * Output: []
 * Example 3:
 * Input: nums = [0]
 * Output: []
 * 
 * Constraints:
 * 
 * 	0 <= nums.length <= 3000
 * 	-105 <= nums[i] <= 105
 ******************************************************************************************************/

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
