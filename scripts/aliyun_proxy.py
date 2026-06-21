"""Request relay to run inside mainland China. Put the target URL in X-Proxy-Url
(or ?__url=) and it replays the request from China. No credentials stored."""

from flask import Flask, request, Response
import requests

app = Flask(__name__)

TIMEOUT = 15

# Hop-by-hop headers + Host + our control header: not forwarded. Accept-Encoding is
# dropped so requests returns decoded bytes.
SKIP_REQUEST = {
    "host", "connection", "keep-alive", "proxy-authenticate", "proxy-authorization",
    "te", "trailers", "transfer-encoding", "upgrade", "content-length",
    "accept-encoding", "x-proxy-url",
}
SKIP_RESPONSE = {"content-encoding", "transfer-encoding", "connection", "content-length"}

_METHODS = ["GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"]


@app.route("/", defaults={"path": ""}, methods=_METHODS)
@app.route("/<path:path>", methods=_METHODS)
def proxy(path):
    target = request.headers.get("X-Proxy-Url") or request.args.get("__url")
    if not target:
        return Response("Missing X-Proxy-Url header or __url query param", status=400)

    fwd_headers = {k: v for k, v in request.headers.items()
                   if k.lower() not in SKIP_REQUEST}

    try:
        upstream = requests.request(
            method=request.method,
            url=target,
            headers=fwd_headers,
            data=request.get_data(),
            timeout=TIMEOUT,
            allow_redirects=True,
        )
    except requests.RequestException as e:
        return Response(f"Upstream request failed: {e}", status=502)

    headers = [(k, v) for k, v in upstream.headers.items()
               if k.lower() not in SKIP_RESPONSE]
    return Response(upstream.content, status=upstream.status_code, headers=headers)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=9000)
