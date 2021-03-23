# Source : https://leetcode-cn.com/problems/longest-common-prefix/
# Author : jkbs487
# Date   : 2021-03-14

##################################################################################################### 
#
# Write a function to find the longest common prefix string amongst an array of strings.
# 
# If there is no common prefix, return an empty string "".
# 
# Example 1:
# 
# Input: strs = ["flower","flow","flight"]
# Output: "fl"
# 
# Example 2:
# 
# Input: strs = ["dog","racecar","car"]
# Output: ""
# Explanation: There is no common prefix among the input strings.
# 
# Constraints:
# 
# 	0 <= strs.length <= 200
# 	0 <= strs[i].length <= 200
# 	strs[i] consists of only lower-case English letters.
#####################################################################################################

class Solution:
    def longestCommonPrefix(self, strs: List[str]) -> str:
        """
        use max and min of Python to sort list strs, commparing
        the common prefix of the largest and smallest str is the
        global common prefix.
        """
        if not strs:
            return ""
        s1 = min(strs)
        s2 = max(strs)
        for i, x in enumerate(s1):
            if x != s2[i]:
                return s2[:i]
        return s1
