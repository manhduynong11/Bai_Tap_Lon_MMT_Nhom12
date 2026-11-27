# Quy ước dự án ET4230 - Đề 12: Ứng dụng Chat đa người dùng qua Socket

## 1. Môi trường & Công nghệ bắt buộc
- Hệ điều hành: Windows (KHÔNG dùng máy ảo, KHÔNG dùng thư viện chỉ chạy trên Linux).
- Ngôn ngữ lõi: C++17, sử dụng Winsock2 (`winsock2.h`, `ws2tcpip.h`), đa luồng bằng `std::thread`, `std::mutex`, đo thời gian bằng `std::chrono::steady_clock`.
- Lệnh biên dịch chuẩn: `g++ -std=c++17 -O2 <file.cpp> -o <file.exe> -lws2_32`
- Thư viện JSON: dùng `src/json.hpp` (`nlohmann/json`, header-only).
- Python trong `.venv`: CHỈ dùng cho script xử lý dữ liệu và vẽ biểu đồ (`plots/plot.py` với `matplotlib`, `pandas`).

## 2. Yêu cầu giao thức & Chức năng (Đề 12)
- Framing TCP bắt buộc: Length-Prefixed Framing = `[4 byte độ dài payload theo Big-Endian (htonl/ntohl)]` + `[Payload JSON UTF-8]`. Luôn dùng `send_all` và `recv_all` để chống dính/cắt gói.
- Chức năng bắt buộc:
  1. Đăng ký / Đăng nhập mức demo (lưu file cục bộ, hash đơn giản).
  2. Chat nhóm (theo phòng), nhắn riêng (DM), lấy danh sách thành viên online.
  3. Phân quyền căn bản (RBAC): `ADMIN` (kick, mute, broadcast) và `USER` (chat thường).
  4. Xử lý mất kết nối: Heartbeat (`PING`/`PONG`) phía Server + tự động kết nối lại (`RECONNECT`) phía Client.

## 3. Yêu cầu thí nghiệm & Dữ liệu
- Phải xây dựng `src/benchmark.cpp` chạy tự động tối thiểu 03 kịch bản:
  - KB1: Thay đổi số client đồng thời (tải nhẹ/trung bình/cao: 10, 50, 100, 200).
  - KB2: Thay đổi kích thước thông điệp (64B, 512B, 2KB, 8KB).
  - KB3: Giả lập độ trễ mạng (0, 50, 150ms) và mất kết nối đột ngột (5%, 15%).
- Mỗi điểm đo lặp tối thiểu 05 lần với seed khác nhau; lưu dữ liệu thô thực tế ra `results/*.csv`.
- Chỉ tiêu đo: thông lượng (msg/s), độ trễ trung bình & P95 (ms), tỷ lệ gửi thành công (%), thời gian & tỷ lệ khôi phục.
- TUYỆT ĐỐI KHÔNG tự bịa dữ liệu hoặc tạo kết quả giả.

## 4. Cấu trúc thư mục
- `src/`: mã nguồn C++ (`net_compat.hpp`, `protocol.hpp`, `server.cpp`, `client.cpp`, `benchmark.cpp`).
- `configs/`: file cấu hình JSON (IP, port, timeout, thông số benchmark).
- `results/`: file `.csv` dữ liệu đo thực tế.
- `plots/`: `plot.py` và ảnh biểu đồ `.png` (bắt buộc có tiêu đề, chú giải, đơn vị, error bar).
- `demo/`: ảnh chụp, log, file bắt gói tin Wireshark (`.pcapng`).
- `report/`: báo cáo PDF và slide bảo vệ.
