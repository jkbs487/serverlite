/*
 * @lc app=leetcode.cn id=16 lang=cpp
 *
 * [16] 最接近的三数之和
 */

// @lc code=start
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
// @lc code=end

