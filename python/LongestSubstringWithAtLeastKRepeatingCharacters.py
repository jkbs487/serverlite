# Source : https://leetcode-cn.com/problems/longest-substring-with-at-least-k-repeating-characters/
# Author : jkbs487
# Date   : 2021-02-28

##################################################################################################### 
#
# Given a string s and an integer k, return the length of the longest substring of s such that the 
# frequency of each character in this substring is greater than or equal to k.
# 
# Example 1:
# 
# Input: s = "aaabb", k = 3
# Output: 3
# Explanation: The longest substring is "aaa", as 'a' is repeated 3 times.
# 
# Example 2:
# 
# Input: s = "ababbc", k = 2
# Output: 5
# Explanation: The longest substring is "ababb", as 'a' is repeated 2 times and 'b' is repeated 3 
# times.
# 
# Constraints:
# 
# 	1 <= s.length <= 104
# 	s consists of only lowercase English letters.
# 	1 <= k <= 105
#####################################################################################################

class Solution:
    def longestSubstring(self, s: str, k: int) -> int:
        if len(s) < k:
            return 0
        cnt = Counter(s)
        for c in s:
            ans = 0
            if cnt[c] < k:
                return max(self.longestSubstring(t, k) for t in s.split(c))
        return len(s)
