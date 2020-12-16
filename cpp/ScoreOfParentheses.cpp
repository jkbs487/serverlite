// Source : https://leetcode-cn.com/problems/score-of-parentheses/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Given a balanced parentheses string S, compute the score of the string based on the following rule:
 * 
 * 	() has score 1
 * 	AB has score A + B, where A and B are balanced parentheses strings.
 * 	(A) has score 2 * A, where A is a balanced parentheses string.
 * 
 * Example 1:
 * 
 * Input: "()"
 * Output: 1
 * 
 * Example 2:
 * 
 * Input: "(())"
 * Output: 2
 * 
 * Example 3:
 * 
 * Input: "()()"
 * Output: 2
 * 
 * Example 4:
 * 
 * Input: "(()(()))"
 * Output: 6
 * 
 * Note:
 * 
 * 	S is a balanced parentheses string, containing only ( and ).
 * 	2 <= S.length <= 50
 * 
 ******************************************************************************************************/

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
