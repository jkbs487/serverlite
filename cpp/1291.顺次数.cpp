/*
 * @lc app=leetcode.cn id=1291 lang=cpp
 *
 * [1291] 顺次数
 */

// @lc code=start
class Solution {
public:
    vector<int> sequential(int n) {
        vector<int> list;
        for(int i = n; i < 10; i++) {
            int num = 0, cur = i;
            for(int j = 0; j < n; j++) {
                num += pow(10, j) * (cur--);
            }
            list.push_back(num);
        }
        return list;
    }

    vector<int> sequentialDigits(int low, int high) {
        vector<int> ans;
        for(int i = 1; i < 10; i++) {
            vector<int> list = sequential(i);
            for(auto l : list) 
                if(l <= high && l >= low)
                    ans.push_back(l);
        }
        return ans;
    }
};
// @lc code=end

