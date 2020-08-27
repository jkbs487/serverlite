/*
 * @lc app=leetcode.cn id=7 lang=cpp
 *
 * [7] 整数反转
 * 关键字：整数
 */

// @lc code=start
class Solution {
public:
    int reverse(int x) {
        long long r = 0;
        while(x != 0) {
            r = r * 10 + x % 10;
            x = x / 10;
          if(r > INT_MAX || r < INT_MIN) 
            return 0;
        }
        return (int)r;
    }
};
// @lc code=end

