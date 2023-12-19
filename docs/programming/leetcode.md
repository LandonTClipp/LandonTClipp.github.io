---
title: Leetcode
---

## Array/String

### [Merged Sorted Array](https://leetcode.com/problems/merge-sorted-array/) (easy)

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

## Two Pointers

### [Valid Palindrome](https://leetcode.com/problems/valid-palindrome/) (easy)

#### Problem Statement

A phrase is a palindrome if, after converting all uppercase letters into lowercase letters and removing all non-alphanumeric characters, it reads the same forward and backward. Alphanumeric characters include letters and numbers.

Given a string s, return true if it is a palindrome, or false otherwise.

 

Example 1:

    Input: s = "A man, a plan, a canal: Panama"
    Output: true
    Explanation: "amanaplanacanalpanama" is a palindrome.

Example 2:

    Input: s = "race a car"
    Output: false
    Explanation: "raceacar" is not a palindrome.

Example 3:

    Input: s = " "
    Output: true
    Explanation: s is an empty string "" after removing non-alphanumeric characters.
    Since an empty string reads the same forward and backward, it is a palindrome.
 

Constraints:

1. 1 <= s.length <= 2 * 105
2. s consists only of printable ASCII characters.

#### [Solution](https://leetcode.com/problems/valid-palindrome/submissions/1120632597)
##### Intuition

This is a fairly simple problem. We need to normalize the input string to disregard non-alphanumeric characters. There are a few ways you can do this, but I intend to use an approach that utilizes the least amount of data copying.

##### Approach

A naive approach would be to normalize the input string by copying each alphanumeric element into a new string and setting it to its lowercase representation. Then you could iterate over the normalized string and compare it to the opposing end of the string. However, this approach is costly as it requires lots of data copying and computation.

Instead, I used a cursor approach where we iterate over each element of the string. We will `continue` the loop if we found a non-alphanumeric character. Additionally, we keep track of the "opposing index" of the string, what we'll call `oppositeCursor`. In the `for` loop, the `oppositeCursor` is decremented until we find an alphanumeric character. Once that is found, we compare the lower-case representation at `s[i]` and `s[oppositeCursor]` and if they don't match, then it is not a valid palindrome.

#####Complexity
- Time complexity: $O(n)$

- Space complexity: $O(1)$

##### Code

```go
func isPalindrome(s string) bool {
    oppositeCursor := len(s)-1
    for i := 0; i < len(s); i++ {
        if i > oppositeCursor {
            break
        }
        if !isAlphaNumeric(s[i]) {
            continue
        }
        for;!isAlphaNumeric(s[oppositeCursor]) && i<oppositeCursor ; oppositeCursor-- {}
        if i > oppositeCursor || strings.ToLower(string(s[i])) != strings.ToLower(string(s[oppositeCursor])) {
            return false
        }
        oppositeCursor--
    }
    return true
}

func isAlphaNumeric(c byte) bool {
    r := rune(c)
    return unicode.IsLetter(r) || unicode.IsNumber(r)
}

```

## Sliding Window

### Minimum Size Subarray Sum

#### Solution

Naive (brute force):

```go
func minSubArrayLen(target int, nums []int) int {
    minLength := 0

outerloop:
    for left := 0; left < len(nums); left++ {
        cumulativeSum += left
        sum := 0

        for right := left; right < len(nums); right++ {
            sum += nums[right]
            if sum >= target {
                length := right - left + 1
                if minLength == 0 || minLength > length {
                    minLength = length
                }
                continue outerloop
            }
        }
    }
    return minLength
}
```

| status | language | runtime | memory |
|--------|----------|---------|--------|
| Accepted | Go | 2884ms | 8.4MB |

Sliding Window:

```go
func minSubArrayLen(target int, nums []int) int {
    minLength := len(nums) + 1
    sum := 0
    left := 0
    for right := 0; right < len(nums); right++ {
        sum += nums[right]
        for sum >= target {
            minLength = min(minLength, right - left + 1)
            sum -= nums[left]
            left++
        }
    }

    if minLength == len(nums) + 1 {
        return 0
    }
    return minLength
}

```