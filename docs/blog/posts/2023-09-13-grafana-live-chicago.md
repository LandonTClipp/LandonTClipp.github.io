---
date: 2023-09-13
categories:
  - Grafana
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

## Agenda

- Grafana Labs Stack
- loki
- demo
- setup
- breakout 1 - effective troubleshooting and debugging
- Use case: ad-hoc metrics
- Breakout 2 - loki v2 real time and high cardinality metrics
- news/updates
- wrap up

!!! question "What is loki?"
    
    It's a log aggregation and querying platform, similar in some ways to splunk.

## Loki Workshop Breakout

https://github.com/grafana/loki_workshop_breakout

## 4 Types of Observability Data:

1. logs
2. metrics
3. traces
4. profiles

## Understanding SLAs, SLOs, SLIs

- SLA: Service Level Agreement. The agreement you make with your customers that is often contractually guaranteed.
    - Example: 99.5% guaranteed system availability per month
- SLI: Service Level Indicator. A frequently run probe of a user-centric KPIs to measure service or application health.
    - Example: % of successful and performant requests
    - These are the customer-centric metric being measured
- SLO: Service Level Objective. 
    - Example: 99.9% of requests to a web service return without errors.

The SLO is simply the SLI threshold you are striving for.

### SLI Methodology

You can have multiple measurements that comprise your SLI. For example

- Rate: requests per second
- Errors: The number of those requests that are failing
- Duration: The amount of time requests take, distribution of latency measurements

There is a concept of Error Budget, which means how many errors you're willing to tolerate. For example, we may tolerate .1% of errored requests over a 1 month period. We can measure this by splitting between fast burn rates and slow burn rates. 

- Fast burn: 10% of requests fail over a window of the last 2 days
- Slow burn: 2% of requests fail over a window of the last 15 days.

### Grafana SLO - The Easy Button

- Grafana gives you UIs to create an SLO
- Create dashboard showing details of SLO
- A set of multi-window, multi-burn rate alerts
