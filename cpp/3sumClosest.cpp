// Source : https://leetcode-cn.com/problems/3sum-closest/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Given an array nums of n integers and an integer target, find three integers in nums such that the 
 * sum is closest to target. Return the sum of the three integers. You may assume that each input 
 * would have exactly one solution.
 * 
 * Example 1:
 * 
 * Input: nums = [-1,2,1,-4], target = 1
 * Output: 2
 * Explanation: The sum that is closest to the target is 2. (-1 + 2 + 1 = 2).
 * 
 * Constraints:
 * 
 * 	3 <= nums.length <= 10^3
 * 	-10^3 <= nums[i] <= 10^3
 * 	-10^4 <= target <= 10^4
 ******************************************************************************************************/

class Solution {
public:
    int threeSumClosest(vector<int>& nums, int target) {
        sort(nums.begin(), nums.end());
        int first, last = nums.size()-1;
        int subNum = INT_MAX, minNum;
        for(int i = 0; i < nums.size(); i++) {
            first = i+1;
            last = nums.size()-1;
            while(first < last) {
                int temp = nums[i] + nums[first] + nums[last] - target;
                if(abs(temp) < subNum) {
                    minNum = temp + target;  
                    subNum = abs(temp);
                }
                if(temp < 0) first++;
                if(temp > 0) last--;
                if(temp == 0) return minNum;
            }
        }
        return minNum;
    }
};
