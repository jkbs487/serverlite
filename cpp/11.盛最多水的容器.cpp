/*
 * @lc app=leetcode.cn id=11 lang=cpp
 *
 * [11] 盛最多水的容器
 */

// @lc code=start
class Solution {
public:
    int maxArea(vector<int>& height) {
        int ans = 0;
        int left = 0, right = height.size() - 1;
        while(left < right){
            if(height.at(left) <= height.at(right)) {
               ans = max(ans, height.at(left) * (right - left));
               left++;
           } 
            if(height.at(left) > height.at(right)) {
                ans = max(ans, height.at(right) * (right - left));
                right--;
            }
        }
        return ans;
    }
};
// @lc code=end

