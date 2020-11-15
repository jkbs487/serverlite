/*
 * @lc app=leetcode.cn id=402 lang=cpp
 *
 * [402] 移掉K位数字
 * 栈，贪心
 * 出栈条件：
 * 1. k大于零
 * 2. 栈不为空
 * 3. 栈顶元素大于当前元素
 * 最后判断：
 * 1. k如果不为零，去掉k个栈底元素
 * 2. 如果有前置零，去掉前置零
 * 3. 如果栈为空，补一个零
 */

// @lc code=start
class Solution {
public:
    string removeKdigits(string num, int k) {
        string ans;
        if(k == 0) return num;
        deque<char> dq;
        for(auto n : num) {
            while(k-- > 0 && !dq.empty() && dq.back() > n) {
                dq.pop_back();
                k--;
            }
            dq.push_back(n);
        }
        while(k-- > 0) dq.pop_back();
        while(!dq.empty() && dq.front() == '0') dq.pop_front();
        if(dq.size() == 0) dq.push_back('0');
        ans.insert(ans.begin(), dq.begin(), dq.end());
        return ans;
    }
};
// @lc code=end

