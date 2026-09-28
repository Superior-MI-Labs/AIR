#!/usr/bin/env python3

import argparse
import concurrent.futures
import json
import threading
import urllib.error
import urllib.request


def post(base, prompt, tokens, barrier):
    barrier.wait()
    payload = {
        "prompt": prompt,
        "max_tokens": tokens,
        "temperature": 0,
        "stream": False,
    }
    req = urllib.request.Request(
        base + "/v1/completions",
        data=json.dumps(payload).encode(),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=180) as response:
            response.read()
            return response.status
    except urllib.error.HTTPError as error:
        error.read()
        return error.code


def get_json(url):
    with urllib.request.urlopen(url, timeout=10) as response:
        return json.loads(response.read().decode())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", required=True)
    parser.add_argument("--requests", type=int, default=12)
    parser.add_argument("--tokens", type=int, default=256)
    parser.add_argument(
        "--prompt",
        default="Explain bounded inference scheduling and resource ownership in detail. ",
    )
    args = parser.parse_args()

    if args.requests < 3:
        raise SystemExit("--requests must be at least 3")

    base = args.url.rstrip("/")
    barrier = threading.Barrier(args.requests)
    statuses = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.requests) as pool:
        futures = [
            pool.submit(post, base, args.prompt * 8, args.tokens, barrier)
            for _ in range(args.requests)
        ]
        for future in futures:
            statuses.append(future.result())

    runtime = get_json(base + "/runtime")
    result = {
        "requests": args.requests,
        "statuses": statuses,
        "http_200": statuses.count(200),
        "http_503": statuses.count(503),
        "unexpected": [status for status in statuses if status not in (200, 503)],
        "rejected_overload_requests": runtime.get("rejected_overload_requests"),
        "queued_requests": runtime.get("queued_requests"),
        "active_requests": runtime.get("active_requests"),
    }
    print(json.dumps(result, indent=2, sort_keys=True))

    ok = (
        result["http_200"] >= 1
        and result["http_503"] >= 1
        and not result["unexpected"]
        and isinstance(result["rejected_overload_requests"], int)
        and result["rejected_overload_requests"] >= 1
        and result["queued_requests"] == 0
        and result["active_requests"] == 0
    )
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
