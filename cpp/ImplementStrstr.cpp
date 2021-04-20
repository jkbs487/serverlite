// Source : https://leetcode-cn.com/problems/implement-strstr/
// Author : jkbs487
// Date   : 2021-04-20

/***************************************************************************************************** 
 *
 * Implement strStr().
 * 
 * Return the index of the first occurrence of needle in haystack, or -1 if needle is not part of 
 * haystack.
 * 
 * Clarification:
 * 
 * What should we return when needle is an empty string? This is a great question to ask during an 
 * interview.
 * 
 * For the purpose of this problem, we will return 0 when needle is an empty string. This is 
 * consistent to C's strstr() and Java's indexOf().
 * 
 * Example 1:
 * Input: haystack = "hello", needle = "ll"
 * Output: 2
 * Example 2:
 * Input: haystack = "aaaaa", needle = "bba"
 * Output: -1
 * Example 3:
 * Input: haystack = "", needle = ""
 * Output: 0
 * 
 * Constraints:
 * 
 * 	0 <= haystack.length, needle.length <= 5 * 104
 * 	haystack and needle consist of only lower-case English characters.
 ******************************************************************************************************/

//violent recursion
class Solution {
public:
    int strStr(string haystack, string needle) {
        if (needle.empty()) return 0;
        if (haystack.empty() || haystack.size() < needle.size()) return -1;
        int len = needle.size();
        for (int i = 0; i < haystack.size()-len+1; i++) {
            if (haystack.substr(i, len) == needle) {
                return i;
            }
        }
        return -1;
    }
};

//KMP
class Solution {
public:
    int strStr(string haystack, string needle) {
        int n = haystack.size(), m = needle.size();
        if (m == 0) {
            return 0;
        }
        vector<int> pi(m);
        //record pi of needle
        for (int i = 1, j = 0; i < m; i++) {
            while (j > 0 && needle[i] != needle[j]) {
                j = pi[j - 1];
            }
            if (needle[i] == needle[j]) {
                j++;
            }
            pi[i] = j;
        }
        for (int i = 0, j = 0; i < n; i++) {
            while (j > 0 && haystack[i] != needle[j]) {
                j = pi[j - 1];
            }
            if (haystack[i] == needle[j]) {
                j++;
            }
            //first j substring must be needle
            if (j == m) {
                return i - m + 1;
            }
        }
        return -1;
    }
};
