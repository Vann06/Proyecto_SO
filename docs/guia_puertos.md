# WSL2 Port Proxying Guide for Chat Project

This guide explains how to allow external devices (like a Mac or another PC) to connect to a server running inside **WSL2** by using Windows as a network bridge.

## Prerequisites

- You must run these commands in **PowerShell as Administrator**.
- You need your **WSL2 Internal IP** (Run `hostname -I` inside WSL).
- You need your **Server Port** (e.g., `8888`).

---

## 1. Setup: Open the Connection

Run these two commands in PowerShell (Admin) to create the bridge and open the firewall.

### Step A: Create the Port Bridge

Replace `<WSL_IP>` with your `hostname -I` result and `<PORT>` with your server port.

```powershell
netsh interface portproxy add v4tov4 listenport=<PORT> listenaddress=0.0.0.0 connectport=<PORT> connectaddress=<WSL_IP>
```

### Step B: Open Windows Firewall

This tells Windows to stop blocking incoming traffic on that specific port.

```powershell
New-NetFirewallRule -DisplayName "WSL Chat Server" -Direction Inbound -LocalPort <PORT> -Protocol TCP -Action Allow
```

**Note for your friend:** They should connect using your **Windows Host IP** (find it by running `ipconfig` in Windows, usually `192.168.x.x`).

---

## 2. Verification

To see if the bridge is currently active, run:

```powershell
netsh interface portproxy show all
```

---

## 3. Cleanup: Close the Connection

Once you are done testing, it is a **Security Best Practice** to close the bridge and remove the firewall rule.

### Step A: Remove the Port Bridge

```powershell
netsh interface portproxy delete v4tov4 listenport=<PORT> listenaddress=0.0.0.0
```

### Step B: Remove the Firewall Rule

```powershell
Remove-NetFirewallRule -DisplayName "WSL Chat Server"
```

---

## Summary of IPs

| Role                           | IP to Use                   | Command to Find               |
| :----------------------------- | :-------------------------- | :---------------------------- |
| **Your WSL Server**      | `0.0.0.0` (Listen on all) | N/A                           |
| **The Proxy Bridge**     | `<WSL_IP>`                | `hostname -I` (in WSL)      |
| **Your Friend (Client)** | `<Windows_IP>`            | `ipconfig` (in Windows CMD) |
