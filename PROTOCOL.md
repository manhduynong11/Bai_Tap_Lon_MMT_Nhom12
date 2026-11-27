# Đặc tả Giao thức Tầng Ứng dụng — Đề 12 (ET4230)

## 1. Tầng giao vận và cơ chế Framing
- **Giao thức nền:** TCP (IPv4, Winsock2 / POSIX sockets).
- **Cơ chế đóng khung (Length-Prefixed Framing):** Giải quyết hiện tượng dính gói (coalescing) và cắt gói (fragmentation) trên luồng byte TCP.
  - Cấu trúc một khung (Frame): `[Header: 4 byte] + [Payload: N byte]`
  - **Header:** Số nguyên không dấu 32-bit (`uint32_t`) biểu diễn độ dài $N$ (tính bằng byte) của Payload, mã hóa theo chuẩn **Network Byte Order (Big-Endian)** bằng `htonl()` khi gửi và `ntohl()` khi nhận.
  - **Payload:** Chuỗi JSON mã hóa UTF-8.
  - **Giới hạn kích thước:** $1 \le N \le 1{,}048{,}576$ byte (1 MB). Khung có độ dài $N = 0$ hoặc $N > 1\text{ MB}$ sẽ bị từ chối và đóng kết nối để tránh tràn bộ nhớ.

## 2. Cấu trúc gói tin JSON chuẩn
Mọi bản tin trao đổi giữa Client và Server đều dùng chung cấu trúc bao (envelope):

```json
{
  "type": "GROUP_MSG",
  "msg_id": "c1-00042",
  "timestamp_ms": 1764213600125,
  "sender": "duy",
  "target": "general",
  "payload": "Xin chao ca nhom",
  "meta": {}
}