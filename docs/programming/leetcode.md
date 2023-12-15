---
title: Leetcode
---

## Array/String

### [Merged Sorted Array](https://leetcode.com/problems/merge-sorted-array/)

#### Problem Statement

You are given two integer arrays nums1 and nums2, sorted in non-decreasing order, and two integers m and n, representing the number of elements in nums1 and nums2 respectively.

Merge nums1 and nums2 into a single array sorted in non-decreasing order.

The final sorted array should not be returned by the function, but instead be stored inside the array nums1. To accommodate this, nums1 has a length of m + n, where the first m elements denote the elements that should be merged, and the last n elements are set to 0 and should be ignored. nums2 has a length of n.

 

Example 1:

    Input: nums1 = [1,2,3,0,0,0], m = 3, nums2 = [2,5,6], n = 3
    Output: [1,2,2,3,5,6]
    Explanation: The arrays we are merging are [1,2,3] and [2,5,6].
    The result of the merge is [1,2,2,3,5,6] with the underlined elements coming from nums1.

Example 2:

    Input: nums1 = [1], m = 1, nums2 = [], n = 0
    Output: [1]
    Explanation: The arrays we are merging are [1] and [].
    The result of the merge is [1].

Example 3:

    Input: nums1 = [0], m = 0, nums2 = [1], n = 1
    Output: [1]
    Explanation: The arrays we are merging are [] and [1].
    The result of the merge is [1].

 Note that because m = 0, there are no elements in nums1. The 0 is only there to ensure the merge result can fit in nums1.
 

Constraints:

- nums1.length == m + n
- nums2.length == n
- 0 <= m, n <= 200
- 1 <= m + n <= 200
- -109 <= nums1[i], nums2[j] <= 109
 

Follow up: Can you come up with an algorithm that runs in O(m + n) time?

#### [Solution](https://leetcode.com/problems/merge-sorted-array/submissions/1120616479)

##### Intuition
Because we know that both arrays come pre-sorted, we can take advantage of the fact that `i+1` will always be >= `i` for each array.

##### Approach
I decide to use a cursor-based approach. We will allocate a new slice of size `m+n` and use two individual cursors that point into each array respectively. Whenever we "consume" an element from either array, we'll increment the corresponding cursor. We'll need to account for edge cases where we have consumed all available values in a particular array.

##### Complexity
- Time complexity: $O(m+n)$

- Space complexity: $O(m+n)$

##### Code
```go
func merge(nums1 []int, m int, nums2 []int, n int)  {
    merged := make([]int, m+n)
    m_cursor := 0
    n_cursor := 0
    for i := 0; i < m+n; i++ {
        if m_cursor >= m {
            merged[i] = nums2[n_cursor]
            n_cursor++
            continue
        }
        if n_cursor >= n {
            merged[i] = nums1[m_cursor]
            m_cursor++
            continue
        }


        // Asumption in this block is that both m_cursor < m and
        // n_cursor < n. So we need to account for the cases where
        // we've consumed all the values from one or the other
        if nums1[m_cursor] < nums2[n_cursor] {
            merged[i] = nums1[m_cursor]
            m_cursor++
        } else if nums2[n_cursor] < nums1[m_cursor] {
            merged[i] = nums2[n_cursor]
            n_cursor++
        } else {
            // they must be equal, so pick an element arbitrarily
            merged[i] = nums1[m_cursor]
            m_cursor++
        }
    }
    for i := 0; i < m+n; i++ {
        nums1[i] = merged[i]
    }
}

```


