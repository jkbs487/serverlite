/*
 * @lc app=leetcode.cn id=8 lang=cpp
 *
 * [8] 字符串转换整数 (atoi)
 */

// @lc code=start
class Solution {
public:
    int myAtoi(string str) {
        long long ans = 0;
        int index = 0, symbol = 1;
        while(str[index] == ' ') index++;
        if(str[index] == '-') {
            index++;
            symbol = 0;
        } 
        else if(str[index] == '+') index++;  
        if(str[index] > '9' || str[index] < '0') return 0;
        for(int i = index; i < str.size(); i++) {
            if(str[i] <= '9' && str[i] >= '0') 
                ans = 10*ans + (str[i] - '0');
            else break;
            if(ans >= (long long)INT_MAX) break;
        }
        if(symbol) return (int)min(ans, (long long)INT_MAX);
        else return (int)max(-ans, (long long)INT_MIN);
    }
};
// @lc code=end

