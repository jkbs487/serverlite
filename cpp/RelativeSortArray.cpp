// Source : https://leetcode-cn.com/problems/relative-sort-array/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Given two arrays arr1 and arr2, the elements of arr2 are distinct, and all elements in arr2 are 
 * also in arr1.
 * 
 * Sort the elements of arr1 such that the relative ordering of items in arr1 are the same as in arr2. 
 *  Elements that don't appear in arr2 should be placed at the end of arr1 in ascending order.
 * 
 * Example 1:
 * Input: arr1 = [2,3,1,3,2,4,6,7,9,2,19], arr2 = [2,1,4,3,9,6]
 * Output: [2,2,2,1,4,3,3,9,6,7,19]
 * 
 * Constraints:
 * 
 * 	1 <= arr1.length, arr2.length <= 1000
 * 	0 <= arr1[i], arr2[i] <= 1000
 * 	All the elements of arr2 are distinct.
 * 	Each arr2[i] is in arr1.
 ******************************************************************************************************/

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
