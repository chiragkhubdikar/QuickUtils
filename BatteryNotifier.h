/*
* Developer: Chirag Khubdikar
* 
* Date: 25th September 2022
* 
* Class: CBatteryNotifier
* 
* Description:
* This is a quick solution to the Laptop overcharging issue.
* We receive notification when the Laptop battery is low, but not when the
* Laptop battery is about to reach 100%. This class with help us to monitor the 
* the battery level based on our needs. The default is set to 90%
* 
* Code Usage:
* In Main function, call in following fashion,
* CBatteryNotifier::instance().processCommandLine(argc,argv).startBatteryMonitoring();
* or
* CBatteryNotifier::instance().startBatteryMonitoring();
* 
* Command-Line Usage:
* BatteryNotifier.exe /? (Help)
* BatteryNotifier.exe <5-99> (Default value is:90%)
*/
#pragma once
#ifdef WIN32
#include <Windows.h>
#endif // WIN32
#include <thread>
#include <string>
#include <atomic>
#include <algorithm>
#include <memory>
#include <sstream>
#include <bit>

#ifndef TSTRING
#define TSTRING
#ifdef UNICODE
using tstring = std::wstring;
using tstringstream = std::wstringstream;
using tchar = wchar_t;
#define __TEXT(x) L ## x
#define to_tstring std::to_wstring
#else
using tstring = std::string;
using tstringstream = std::stringstream;
using tchar = char;
#define __TEXT(x) x
#define to_tstring std::to_string
#endif // UNICODE
#endif // TSTRING

constexpr auto kSleepForASecinMS = 1000;
constexpr auto kMonitorIntervalInMS = kSleepForASecinMS * 30; // 30 seconds
const auto kHighLimitBeepFrequency = 530;
const auto kBeepDurationInMS = 300;
const auto kDefaultMaxBatteryLimit = 90;
const auto kMessageBoxTitle = __TEXT("Battery Notification");
const auto kAppHelpText = __TEXT("\n\nUsage:\n********\n\nBatteryNotifier.exe /? (Help)\nBatteryNotifier.exe <5-99> (Default value is:90%)");
class CBatteryNotifier
{
public:
	/*
	* Returns a Singleton instance of CBatteryNotifier
	*/
	inline static CBatteryNotifier& instance()
	{
		static CBatteryNotifier m_instance;
		return m_instance;
	}

	/*
	* This function is used to process command line parameters. This is an optional function.
	* @param argc: holds number of arguments.
	* @param argv: holds array of arguments
	* @return returns object of the same class
	*/
	CBatteryNotifier& processCommandLine(const int& argc, TCHAR* argv[])
	{
		if (2 != argc) {
			return *this;
		}
		auto param = tstring(argv[1]);
		if (0 == param.compare(__TEXT("/?"))) {
			notifyUser(kAppHelpText, false);
			m_bStartMonitoring = false;
			return *this;
		}
		else if (isNumeric(param)) {
			auto val = tstringstream(argv[1]);
			auto percent = unsigned int(0);
			val >> percent;
			if (5 > percent && 99 < percent) {
				notifyUser(__TEXT("monitoring value should be in range 5-99"), false);
				m_bStartMonitoring = false;
				return *this;
			}
			m_HigherLimit = percent;
		}
		else {
			notifyUser(tstring(__TEXT("Invalid param: ")) + argv[1] + kAppHelpText, false);
			m_bStartMonitoring = false;
			return *this;
		}
		return *this;
	}

	/*
	* This Function with start battery monitoring for Higher value.
	*/
	void startBatteryMonitoring()
	{
		if (!m_bStartMonitoring) { return; }
		m_HigherLimit = m_HigherLimit == 0 ? kDefaultMaxBatteryLimit : m_HigherLimit;
		notifyUser(tstring(__TEXT("Battery Monitoring started for ")) + to_tstring(m_HigherLimit) + __TEXT(" %"),false);
		do
		{
			monitorHigherLimit();
			::Sleep(kMonitorIntervalInMS);
		} while (true);
	}
private:
	/*
	* This Function will check if the entered string is a number.
	* @return true if the string is numeric, else false
	*/
	bool isNumeric(const tstring& str)
	{
		auto it = std::find_if(str.begin(), str.end(), [](const tchar& c) {
			return !std::isdigit(c);
			});
		return !str.empty() && it == str.end();
	}

	/*
	* This Function will display a message box and optionally Beep
	* @param message: The message that has to be displayed in the message box
	* @param bBeep: boolean value, default value is true
	*/
	void notifyUser(const tstring& message, const bool bBeep = true)
	{
#ifdef WIN32
		std::atomic_bool bContinueBeep(true);
		m_BeepThread = bBeep ? std::make_unique<std::thread>([&]() {
			while (bContinueBeep)
			{
				::Beep(kHighLimitBeepFrequency, kBeepDurationInMS);
				::Sleep(kSleepForASecinMS);
			}
			}) : nullptr;
		::MessageBox(::GetActiveWindow(), message.c_str(), kMessageBoxTitle, MB_OK | MB_TOPMOST);
		bContinueBeep = false;
		if (m_BeepThread.get()) {
			m_BeepThread->join();
			m_BeepThread.reset();
		}
#endif // WIN32
	}

	/*
	* This function will monitor the Higher limit value
	*/
	void monitorHigherLimit()
	{
#ifdef WIN32
		auto powerStatus = SYSTEM_POWER_STATUS();
		if (::GetSystemPowerStatus(&powerStatus)) {
			if (1 == (char)powerStatus.ACLineStatus && 0 == m_NextHighLimit && m_HigherLimit <= (char)powerStatus.BatteryLifePercent) {
				notifyUser(tstring(__TEXT("Please remove the charger....\nBattery percentage is >= ")) + to_tstring(m_HigherLimit) + __TEXT("%"));
				if (m_HigherLimit <= (char)powerStatus.BatteryLifePercent) {
					m_NextHighLimit = powerStatus.BatteryLifePercent;
				}
			}
			else {
				if (m_NextHighLimit > (char)powerStatus.BatteryLifePercent && m_HigherLimit > (char)powerStatus.BatteryLifePercent) {
					m_NextHighLimit = 0;
				}
				else if (1 == (char)powerStatus.ACLineStatus && 0 < m_NextHighLimit && m_NextHighLimit < (char)powerStatus.BatteryLifePercent) {
					notifyUser(tstring(__TEXT("You have not removed the charger...Please remove it....\nBattery percentage is exceeding ")) + to_tstring((char)powerStatus.BatteryLifePercent) + __TEXT(" % > ") + to_tstring(m_HigherLimit) + __TEXT("%"));
					m_NextHighLimit = powerStatus.BatteryLifePercent;
				}
			}
		}
#endif // WIN32
	}
private:
	/*Default constructor declared private*/
	CBatteryNotifier() = default;

	/*Copy Constructor declared deleted*/;
	CBatteryNotifier(CBatteryNotifier&) = delete;

	/*Copy operator declared deleted*/
	CBatteryNotifier& operator=(CBatteryNotifier&) = delete;

	/*Move Constructor declared deleted*/
	CBatteryNotifier(CBatteryNotifier&&) = delete;

	/*Move operator declared deleted*/;
	CBatteryNotifier&& operator=(CBatteryNotifier&&) = delete;

private:
	bool m_bStartMonitoring = true;
	unsigned int m_HigherLimit = 0, m_NextHighLimit = 0;
	std::unique_ptr<std::thread> m_BeepThread = nullptr;
};
