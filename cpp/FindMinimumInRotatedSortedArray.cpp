// Source : https://leetcode-cn.com/problems/find-minimum-in-rotated-sorted-array/
// Author : jkbs487
// Date   : 2020-12-19

/***************************************************************************************************** 
 *
 * Suppose an array of length n sorted in ascending order is rotated between 1 and n times. For 
 * example, the array nums = [0,1,2,4,5,6,7] might become:
 * 
 * 	[4,5,6,7,0,1,2] if it was rotated 4 times.
 * 	[0,1,2,4,5,6,7] if it was rotated 7 times.
 * 
 * Notice that rotating an array [a[0], a[1], a[2], ..., a[n-1]] 1 time results in the array [a[n-1], 
 * a[0], a[1], a[2], ..., a[n-2]].
 * 
 * Given the sorted rotated array nums, return the minimum element of this array.
 * 
 * Example 1:
 * 
 * Input: nums = [3,4,5,1,2]
 * Output: 1
 * Explanation: The original array was [1,2,3,4,5] rotated 3 times.
 * 
 * Example 2:
 * 
 * Input: nums = [4,5,6,7,0,1,2]
 * Output: 0
 * Explanation: The original array was [0,1,2,4,5,6,7] and it was rotated 4 times.
 * 
 * Example 3:
 * 
 * Input: nums = [11,13,15,17]
 * Output: 11
 * Explanation: The original array was [11,13,15,17] and it was rotated 4 times. 
 * 
 * Constraints:
 * 
 * 	n == nums.length
 * 	1 <= n <= 5000
 * 	-5000 <= nums[i] <= 5000
 * 	All the integers of nums are unique.
 * 	nums is sorted and rotated between 1 and n times.
 ******************************************************************************************************/

//dichotomy
class Solution {
public:
    int findMin(vector<int>& nums) {
        int len = nums.size();
        int left = 0, right = len - 1;
        //no rotation
        if(nums[left] <= nums[right]) { 
            return nums[left];
        }
        //rotation, dichotomy
        while(left <= right) {
            int mid = (right + left) / 2;
            //if middle num less than left element, this is answer
            if(mid - 1 >= 0 && nums[mid] < nums[mid-1]) {
                return nums[mid];
            }
            //if middle num greater than right element, this is answer
            if(mid + 1 < len && nums[mid] > nums[mid+1]) {
                return nums[mid+1];
            }
            //if left num greater than middle num
            if(nums[left] >= nums[mid]) {
                right = mid;
            }
            //if left num less than middle num
            else if(nums[left] < nums[mid]) {
                left = mid;
            }
        }
        return 0;
    }
