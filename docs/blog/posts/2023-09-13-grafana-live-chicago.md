---
date: 2023-09-13
categories:
  - grafana
title: Grafana Live | Chicago 2023
---

# Grafana Live | Chicago 2023

These are my notes from the GrafanaLive workshop held in the Google offices in Chicago.

<!-- more -->

## PromQL Basics

The workshop requires familiarizing one's self with PromQL. Many of the following notes will be copy-pasted.

https://prometheus.io/docs/prometheus/latest/querying/basics/

### Expression result types

- Instant vector - a set of time series containing a single sample for each time series, all sharing the same timestamp
- Range vector - a set of time series containing a range of data points over time for each time series
- Scalar - a simple numeric floating point value
- String - a simple string value; currently unused

### [Instant vector selectors](https://prometheus.io/docs/prometheus/latest/querying/basics/#instant-vector-selectors)

Using an example metrics name 

```
http_requests_total
```

You can filter based off of specific tags

```
http_requests_total{job="prometheus",group="canary"}
```

You can exclude certain filters

- =: Select labels that are exactly equal to the provided string.
- !=: Select labels that are not equal to the provided string.
- =~: Select labels that regex-match the provided string.
- !~: Select labels that do not regex-match the provided string.

### [Range Vector Selectors](https://prometheus.io/docs/prometheus/latest/querying/basics/#range-vector-selectors)

In this example, we select all the values we have recorded within the last 5 minutes for all time series that have the metric name http_requests_total and a job label set to prometheus:

```
http_requests_total{job="prometheus"}[5m]
```


