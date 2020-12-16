// Source : https://leetcode-cn.com/problems/sequential-digits/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * An integer has sequential digits if and only if each digit in the number is one more than the 
 * previous digit.
 * 
 * Return a sorted list of all the integers in the range [low, high] inclusive that have sequential 
 * digits.
 * 
 * Example 1:
 * Input: low = 100, high = 300
 * Output: [123,234]
 * Example 2:
 * Input: low = 1000, high = 13000
 * Output: [1234,2345,3456,4567,5678,6789,12345]
 * 
 * Constraints:
 * 
 * 	10 <= low <= high <= 10^9
 ******************************************************************************************************/

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
