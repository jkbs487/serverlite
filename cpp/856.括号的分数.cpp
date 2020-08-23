/*
 * @lc app=leetcode.cn id=856 lang=cpp
 *
 * [856] 括号的分数
 */

// @lc code=start
class Solution {
public:
    int scoreOfParentheses(string S) {
        stack<int> s;
        for(auto c : S) {
            if(c == '(') 
                s.push(0);
            else {
                int n;
                if(!s.empty()){
                    n = s.top();
                    s.pop();
                }
                else n = 0;
                int m;
                if(!s.empty()) {
                    m = s.top();
                    s.pop();
                } 
                else m = 0;
                s.push(m + max(2*n, 1));
            }
        }
        return s.top();
    }
};
// @lc code=end

