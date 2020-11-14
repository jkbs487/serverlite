/*
 * @lc app=leetcode.cn id=1122 lang=cpp
 *
 * [1122] 数组的相对排序
 * 排序
 * 待比较元素a,b
 * 情况1：a, b都在arr2中，则判断a和b的下标
 * 情况2：a, b只有一个在arr2中，在arr2中的最大
 * 情况3：a, b都不在arr2中，比较a和b的值
 */

// @lc code=start
class Solution {
public:
    vector<int> relativeSortArray(vector<int>& arr1, vector<int>& arr2) {
        vector<int> ans;
        unordered_map<int, int> rank;
        for(int i = 0; i < arr2.size(); i++) 
            rank[arr2[i]] = i;
        auto cmp = [&](int a, int b) {
            if(rank.count(a) && rank.count(b)) 
                return rank[a] < rank[b];
            else if(rank.count(a))
                return true;
            else if(rank.count(b)) 
                return false;
            else
                return a < b;
        };
        sort(arr1.begin(), arr1.end(), cmp);
        return arr1;
    }
};
// @lc code=end

