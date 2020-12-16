// Source : https://leetcode-cn.com/problems/word-pattern/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Given a pattern and a string s, find if s follows the same pattern.
 * 
 * Here follow means a full match, such that there is a bijection between a letter in pattern and a 
 * non-empty word in s.
 * 
 * Example 1:
 * 
 * Input: pattern = "abba", s = "dog cat cat dog"
 * Output: true
 * 
 * Example 2:
 * 
 * Input: pattern = "abba", s = "dog cat cat fish"
 * Output: false
 * 
 * Example 3:
 * 
 * Input: pattern = "aaaa", s = "dog cat cat dog"
 * Output: false
 * 
 * Example 4:
 * 
 * Input: pattern = "abba", s = "dog dog dog dog"
 * Output: false
 * 
 * Constraints:
 * 
 * 	1 <= pattern.length <= 300
 * 	pattern contains only lower-case English letters.
 * 	1 <= s.length <= 3000
 * 	s contains only lower-case English letters and spaces ' '.
 * 	s does not contain any leading or trailing spaces.
 * 	All the words in s are separated by a single space.
 ******************************************************************************************************/

class Solution {
public:
    bool wordPattern(string pattern, string s) {
        unordered_map<char, string> pattern2words;
        unordered_map<string, char> words2pattern;
        int len = s.size();
        int i = 0;
        for(auto p : pattern) {
            if(i >= len) return false;
            int j = i;
            while(j < len && s[j] != ' ') j++;
            string word = s.substr(i, j - i);
            if(pattern2words.count(p) > 0 && pattern2words[p] != word)
                return false;
            if(words2pattern.count(word) > 0 && words2pattern[word] != p) 
                return false;
            pattern2words[p] = word;
            words2pattern[word] = p;
            i = j + 1;
        }
        return i >= len;
    }
};
