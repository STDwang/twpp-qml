#include <memory>
#include <QQmlContext>
#include <QByteArray>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QBuffer>
#include <QDir>
#include "simpleds.hpp"
//#include "scandialog.hpp"
#include "twglue.hpp"
#include "camerasever.h"
using namespace Twpp;
using namespace std::placeholders;

TWPP_ENTRY(SimpleDs)

static constexpr const Identity srcIdent(
	Version(1, 0, Language::English, Country::CzechRepublic, "v1.0"),
	DataGroup::Image,
	"Martin Richter",
	"Examples",
	"Simple TWPP data source"
#if defined(_MSC_VER)
	" MSVC"
#elif defined(__GNUC__)
	" GCC"
#elif defined(__clang__)
	" CLang"
#endif
);

// 让我们只模拟两个轴的统一分辨率
static constexpr UInt32 RESOLUTION = 85;

static int argc = 0;
static char** argv = nullptr;
static QByteArray bmpData;
static std::unique_ptr<QApplication>    application;
static QtCamera* camersever;
static std::unique_ptr<QQmlApplicationEngine> engine;
#if TWPP_DETAIL_OS_WIN
#endif

const Identity& SimpleDs::defaultIdentity() noexcept {
	// 请记住，我们返回一个引用，因此不能将标识放在此方法的堆栈中
	return srcIdent;
}

Result SimpleDs::call(const Identity& origin, DataGroup dg, Dat dat, Msg msg, void* data) {
//    qDebug()<<"call";
	try {
		// 我们几乎可以覆盖 SourceFromThis 中的任何内容，甚至是最顶层的源实例调用
		return Base::call(origin, dg, dat, msg, data);
	}
	catch (const CapabilityException&) {
		return badValue();
	}
}

// 一些辅助函数来处理能力的东西
template<typename T>
static Result oneValGet(Msg msg, Capability& data, const T& value) {
	switch (msg) {
	case Msg::Get:
	case Msg::GetCurrent:
	case Msg::GetDefault:
		data = Capability::createOneValue(data.type(), value);
		return {};

	default:
		return { ReturnCode::Failure, ConditionCode::CapBadOperation };
	}
}

template<typename T>
static Result enmGet(Msg msg, Capability& data, const T& value) {
	switch (msg) {
	case Msg::Get:
		data = Capability::createEnumeration(data.type(), { value });
		return {};
	case Msg::GetCurrent:
	case Msg::GetDefault:
		data = Capability::createOneValue(data.type(), value);
		return {};

	default:
		return { ReturnCode::Failure, ConditionCode::CapBadOperation };
	}
}

template<typename T>
static Result oneValGetSet(Msg msg, Capability& data, T& value, const T& def) {
	switch (msg) {
	case Msg::Reset:
		value = def;
		// fallthrough
	case Msg::Get:
	case Msg::GetCurrent:
		data = Capability::createOneValue(data.type(), value);
		return {};

	case Msg::GetDefault:
		data = Capability::createOneValue(data.type(), def);
		return {};

	case Msg::Set:
		value = data.currentItem<T>();
		return {};

	default:
		return { ReturnCode::Failure, ConditionCode::CapBadOperation };
	}
}

template<typename T>
static Result oneValGetSetConst(Msg msg, Capability& data, const T& def) {
	switch (msg) {
	case Msg::Get:
	case Msg::GetCurrent:
	case Msg::GetDefault:
	case Msg::Reset:
		data = Capability::createOneValue(data.type(), def);
		return {};

	case Msg::Set:
		return data.currentItem<T>() == def ?
			Result() : Result(ReturnCode::Failure, ConditionCode::BadValue);

	default:
		return { ReturnCode::Failure, ConditionCode::CapBadOperation };
	}
}

template<typename T>
static Result enmGetSetConst(Msg msg, Capability& data, const T& def) {
	switch (msg) {
	case Msg::Get:
		data = Capability::createEnumeration(data.type(), { def });
		return {};

	case Msg::GetCurrent:
	case Msg::GetDefault:
	case Msg::Reset:
		data = Capability::createOneValue(data.type(), def);
		return {};

	case Msg::Set:
		return data.currentItem<T>() == def ?
			Result() : Result(ReturnCode::Failure, ConditionCode::BadValue);

	default:
		return { ReturnCode::Failure, ConditionCode::CapBadOperation };
	}
}

Result SimpleDs::capCommon(const Identity&, Msg msg, Capability& data) {
	auto it = m_caps.find(data.type());
	if (it != m_caps.end()) {
		return (it->second)(msg, data);
	}

	return capUnsupported();
}

Result SimpleDs::capabilityGet(const Identity& origin, Capability& data) {
	return capCommon(origin, Msg::Get, data);
}

Result SimpleDs::capabilityGetCurrent(const Identity& origin, Capability& data) {
	return capCommon(origin, Msg::GetCurrent, data);
}

Result SimpleDs::capabilityGetDefault(const Identity& origin, Capability& data) {
	return capCommon(origin, Msg::GetDefault, data);
}

Result SimpleDs::capabilityQuerySupport(const Identity&, Capability& data) {
	auto it = m_query.find(data.type());
	MsgSupport sup = it != m_query.end() ? it->second : msgSupportEmpty;
	data = Capability::createOneValue(data.type(), sup);
	return success();
}

Result SimpleDs::capabilityReset(const Identity& origin, Capability& data) {
	return capCommon(origin, Msg::Reset, data);
}

Result SimpleDs::capabilityResetAll(const Identity& origin) {
	for (auto& pair : m_query) {
		if ((pair.second & MsgSupport::Reset) != msgSupportEmpty) {
			Capability dummyCap(pair.first);
			capCommon(origin, Msg::Reset, dummyCap);
		}
	}

	return success();
}

Result SimpleDs::capabilitySet(const Identity& origin, Capability& data) {
	return capCommon(origin, Msg::Set, data);
}

Result SimpleDs::eventProcess(const Identity&, Event& event) {
	// Qt needs to process its events, otherwise the GUI will appear frozen
	// this is Windows-only method, Linux and macOS behave differently
	// Qt 需要处理它的事件，否则 GUI 会出现冻结
	// 这是仅限 Windows 的方法，Linux 和 macOS 的行为不同
	if (static_cast<bool>(application)) {
		// QApplication::processEvents(); - TODO: needs more investigation; results in freeze when attempting to scan using old DSM
		QApplication::sendPostedEvents();
	}

	event.setMessage(Msg::Null);
	return { ReturnCode::NotDsEvent, ConditionCode::Success };
}

Result SimpleDs::identityOpenDs(const Identity&) {
	// init caps
	// there are caps a minimal source must support
	// query -> says which operations a cap supports
	// caps  -> has handler for each specific cap
	// 有一个最小源必须支持的上限
	// 查询 -> 说明上限支持哪些操作
	// caps -> 对每个特定的 cap 都有处理程序
	m_query[CapType::SupportedCaps] = msgSupportGetAll;
	m_caps[CapType::SupportedCaps] = [this](Msg msg, Capability& data) {
		switch (msg) {
		case Msg::Get:
		case Msg::GetCurrent:
		case Msg::GetDefault: {
			data = Capability::createArray<CapType::SupportedCaps>(m_caps.size());
			auto arr = data.array<CapType::SupportedCaps>();
			UInt32 i = 0;
			for (const auto& kv : m_caps) {
				arr[i] = kv.first;
				i++;
			}

			return success();
		}

		default:
			return capBadOperation();
		}
	};

	m_query[CapType::UiControllable] = msgSupportGetAll;
	m_caps[CapType::UiControllable] = std::bind(enmGet<Bool>, _1, _2, Bool(true));

	m_query[CapType::DeviceOnline] = msgSupportGetAll;
	m_caps[CapType::DeviceOnline] = std::bind(enmGet<Bool>, _1, _2, Bool(true));

	m_query[CapType::XferCount] = msgSupportGetAllSetReset;
	m_caps[CapType::XferCount] = [this](Msg msg, Capability& data) -> Result {
		if (msg == Msg::Set) {
			auto item = data.currentItem<Int16>();
			if (item > 1 || item < -1) {
				return badValue();
			}
		}

		auto ret = oneValGetSet<Int16>(msg, data, m_capXferCount, -1);
		if (Twpp::success(ret) && m_capXferCount == 0) {
			m_capXferCount = -1;
			return { ReturnCode::CheckStatus, ConditionCode::BadValue };
		}

		return ret;
	};

	m_query[CapType::ICompression] = msgSupportGetAllSetReset;
	m_caps[CapType::ICompression] = std::bind(enmGetSetConst<Compression>, _1, _2, Compression::None);


	m_query[CapType::IBitDepth] = msgSupportGetAllSetReset;
	m_caps[CapType::IBitDepth] = std::bind(enmGetSetConst<UInt16>, _1, _2, UInt16(header()->biBitCount));

	m_query[CapType::IBitOrder] = msgSupportGetAllSetReset;
	m_caps[CapType::IBitOrder] = std::bind(enmGetSetConst<BitOrder>, _1, _2, BitOrder::MsbFirst);

	m_query[CapType::IPlanarChunky] = msgSupportGetAllSetReset;
	m_caps[CapType::IPlanarChunky] = std::bind(enmGetSetConst<PlanarChunky>, _1, _2, PlanarChunky::Chunky);

	m_query[CapType::IPhysicalWidth] = msgSupportGetAll;
	m_caps[CapType::IPhysicalWidth] = std::bind(oneValGet<Fix32>, _1, _2, Fix32(static_cast<float>(header()->biWidth) / RESOLUTION));

	m_query[CapType::IPhysicalHeight] = msgSupportGetAll;
	m_caps[CapType::IPhysicalHeight] = std::bind(oneValGet<Fix32>, _1, _2, Fix32(static_cast<float>(header()->biHeight) / RESOLUTION));

	m_query[CapType::IPixelFlavor] = msgSupportGetAllSetReset;
	m_caps[CapType::IPixelFlavor] = std::bind(enmGetSetConst<PixelFlavor>, _1, _2, PixelFlavor::Chocolate);

	m_query[CapType::IPixelType] = msgSupportGetAllSetReset;
	m_caps[CapType::IPixelType] = std::bind(enmGetSetConst<PixelType>, _1, _2, PixelType::Rgb);

	m_query[CapType::IUnits] = msgSupportGetAllSetReset;
	m_caps[CapType::IUnits] = std::bind(enmGetSetConst<Unit>, _1, _2, Unit::Inches);

	m_query[CapType::IXferMech] = msgSupportGetAllSetReset;
	m_caps[CapType::IXferMech] = [this](Msg msg, Capability& data) -> Result {
		switch (msg) {
		case Msg::Get:
			data = Capability::createEnumeration<CapType::IXferMech>(
				{ XferMech::Native, XferMech::Memory }, m_capXferMech == XferMech::Native ? 0 : 1, 0);
			return success();

		case Msg::Reset:
			m_capXferMech = XferMech::Native;
			// fallthrough
		case Msg::GetCurrent:
			data = Capability::createOneValue<CapType::IXferMech>(m_capXferMech);
			return success();

		case Msg::GetDefault:
			data = Capability::createOneValue<CapType::IXferMech>(XferMech::Native);
			return success();

		case Msg::Set: {
			auto mech = data.currentItem<CapType::IXferMech>();
			if (mech == XferMech::Native || mech == XferMech::Memory) {
				m_capXferMech = mech;
				return success();
			}
			else {
				return badValue();
			}
		}

		default:
			return capBadOperation();
		}
	};

	m_query[CapType::IXResolution] = msgSupportGetAllSetReset;
	m_caps[CapType::IXResolution] = [](Msg msg, Capability& data) {
		switch (msg) {
		case Msg::Get:
			data = Capability::createEnumeration(data.type(), { Fix32(RESOLUTION) });
			return success();
		case Msg::GetCurrent:
		case Msg::GetDefault:
		case Msg::Reset:
			data = Capability::createOneValue(data.type(), Fix32(RESOLUTION));
			return success();

		case Msg::Set:
			return data.currentItem<Fix32>() == RESOLUTION ?
				success() : badValue();

		default:
			return capBadOperation();
		}
	};

	m_query[CapType::IYResolution] = msgSupportGetAllSetReset;
	m_caps[CapType::IYResolution] = m_caps[CapType::IXResolution];

	m_query[CapType::IXNativeResolution] = msgSupportGetAll;
	m_caps[CapType::IXNativeResolution] = std::bind(enmGet<Fix32>, _1, _2, Fix32(RESOLUTION));

	m_query[CapType::IYNativeResolution] = msgSupportGetAll;
	m_caps[CapType::IYNativeResolution] = m_caps[CapType::IXNativeResolution];
	return success();
}

Result SimpleDs::identityCloseDs(const Identity&) {
	// 如果使用 RAII，则无需显式释放任何资源
	// TWPP 将在此方法之后自行释放整个源
	return success();
}

Result SimpleDs::pendingXfersGet(const Identity&, PendingXfers& data) {
	data.setCount(m_pendingXfers);
	return success();
}

Result SimpleDs::pendingXfersEnd(const Identity&, PendingXfers& data) {
	data.setCount(0);
	return success();
}

Result SimpleDs::pendingXfersReset(const Identity&, PendingXfers& data) {
	data.setCount(0);
	return success();
}

Result SimpleDs::setupMemXferGet(const Identity&, SetupMemXfer& data) {
	qDebug() << "setupMemXferGet";
	auto bpl = bytesPerLine();
	auto max = bpl * static_cast<UInt32>(header()->biHeight);

	data.setMinSize(bpl);
	data.setPreferredSize(max);
	data.setMaxSize(max);
	return success();
}

Result SimpleDs::userInterfaceDisable(const Identity&, UserInterface&) {
	qDebug() << "userInterfaceDisable";
    engine.reset();
	return success();
}

Result SimpleDs::userInterfaceEnable(const Identity&, UserInterface& ui) {
	m_pendingXfers = 1;
	m_memXferYOff = 0;
	if (!ui.showUi()) {
		// this is an exception when we want to set state explicitly, notifyXferReady can be called only in enabled state
		// with hidden UI, the usual workflow DsState::Enabled -> notifyXferReady() -> DsState::XferReady is a single step
		// 当我们要显式设置状态时这是一个异常，notifyXferReady 只能在启用状态下调用使用隐藏的 UI，
		// 通常的工作流程 DsState::Enabled -> notifyXferReady() -> DsState::XferReady 是一个步骤
		setState(DsState::Enabled);
		auto notified = notifyXferReady();
		return Twpp::success(notified) ? success() : bummer();
	}
	HMODULE hmodule = ::GetModuleHandleA("simpleds.ds");
	char szDs[256];
	char szDsLower[256];
	if (!hmodule) qDebug() << "haven't simpleds.ds";
	else {
		::GetModuleFileNameA(hmodule, szDs, sizeof(szDs));
		strcpy_s(szDsLower, sizeof(szDsLower), szDs);
		_strlwr_s(szDsLower, sizeof(szDsLower));
		if (strstr(szDsLower, "\\twain_32\\") || strstr(szDsLower, "\\twain_64\\"))
		{
			char* szToken = strrchr(szDs, '\\');
			if (szToken)
			{
                szToken[0] = 0;
                QCoreApplication::addLibraryPath(szDs);
			}
		}
	}
	application = std::unique_ptr<QApplication>(new QApplication(argc, argv));
	application->setAttribute(Qt::AA_MacPluginApplication, true);
    engine = std::unique_ptr<QQmlApplicationEngine>(new QQmlApplicationEngine());
    //修改工作目录加载链接库
    engine->addImportPath(szDs);
    QString oldPath = QCoreApplication::applicationDirPath();
    QDir::setCurrent(szDs);
    auto scanFunction1 = [this](QImage cap) {
        QBuffer buffer(&bmpData);
        buffer.open(QIODevice::WriteOnly);
        cap.save(&buffer, "BMP");
        bmpData = qUncompress(qCompress(bmpData, -1));
        notifyXferReady();
    };
    auto cancelFunction1 = [this](QString oldTwainPath) {
        //恢复TWain工作目录
        QDir::setCurrent(oldTwainPath);
        engine->exit(-1);
        notifyCloseCancel();
    };
    TwGlue glue1 = { scanFunction1, cancelFunction1 };
    camersever = new QtCamera(QCameraInfo::defaultCamera(), nullptr, glue1);
    camersever->setTwainPath(oldPath);
    engine->rootContext()->setContextProperty("camersever", camersever);
    engine->addImageProvider(QLatin1String("CodeImg"), camersever->m_pImageProvider);
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(engine.get(), &QQmlApplicationEngine::objectCreated, application.get(), [url](QObject* obj, const QUrl& objUrl) {
        if (!obj && url == objUrl) QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine->load(url);

	return success();
}

Result SimpleDs::userInterfaceEnableUiOnly(const Identity&, UserInterface&) {
	// 作为最小来源，我们不支持仅保存设置的 GUI
	return { ReturnCode::Failure, ConditionCode::OperationError };
}

Result SimpleDs::imageInfoGet(const Identity&, ImageInfo& data) {
	qDebug() << "imageInfoGet";
	// 我们的图片不会改变
	auto dib = header();
	data.setBitsPerPixel(static_cast<Int16>(dib->biBitCount));
	data.setHeight(dib->biHeight);
	data.setPixelType(PixelType::Rgb);
	data.setPlanar(false);
	data.setWidth(dib->biWidth);
	data.setXResolution(RESOLUTION);
	data.setYResolution(RESOLUTION);

	data.setSamplesPerPixel(3);
	data.bitsPerSample()[0] = 8;
	data.bitsPerSample()[1] = 8;
	data.bitsPerSample()[2] = 8;

	return success();
}

Result SimpleDs::imageLayoutGet(const Identity&, ImageLayout& data) {
	qDebug() << "imageLayoutGet";
	// 我们的图片不会改变
	auto dib = header();

	data.setDocumentNumber(1);
	data.setFrameNumber(1);
	data.setPageNumber(1);
	data.setFrame(Frame(0, 0, static_cast<float>(dib->biWidth) / RESOLUTION, static_cast<float>(dib->biHeight) / RESOLUTION));
	return success();
}

Result SimpleDs::imageLayoutGetDefault(const Identity& origin, ImageLayout& data) {
	return imageLayoutGet(origin, data);
}

Result SimpleDs::imageLayoutSet(const Identity& origin, ImageLayout& lay) {
	// 我们不支持设置图像框架
	ImageLayout def;
	imageLayoutGetDefault(origin, def);

	return lay.frame() == def.frame() ? success() : badValue();
}

Result SimpleDs::imageLayoutReset(const Identity& origin, ImageLayout& data) {
	return imageLayoutGet(origin, data);
}

Result SimpleDs::imageMemXferGet(const Identity& origin, ImageMemXfer& data) {
	qDebug() << "imageMemXferGet";
	if (!m_pendingXfers) {
		return seqError();
	}

	// 我们可以调用我们的 TWPP 方法，但要小心状态
	SetupMemXfer setup;
	setupMemXferGet(origin, setup);

	// 只是一个简单的存储 BMP 图像
	auto dib = header();
	auto bpl = bytesPerLine();
	auto memSize = data.memory().size();
	if (memSize > setup.maxSize() || memSize < setup.minSize()) {
		return badValue();
	}

	auto maxRows = memSize / bpl;
	auto rows = std::min<UInt32>(maxRows, static_cast<UInt32>(dib->biHeight) - m_memXferYOff);
	if (rows == 0) {
		return seqError(); // 此会话中已传输图像
	}

	data.setBytesPerRow(bpl);
	data.setColumns(static_cast<UInt32>(dib->biWidth));
	data.setRows(rows);
	data.setBytesWritten(rows * bpl);
	data.setXOffset(0);
	data.setYOffset(m_memXferYOff);
	data.setCompression(Compression::None);

	auto lock = data.memory().data();
	char* out = lock.data();

	// 自底向上 BMP -> 自顶向下内存传输
	auto begin = bmpEnd() - (bpl * (m_memXferYOff + 1));
	for (UInt32 i = 0; i < rows; i++) {
		// copy bytes
		std::copy(begin, begin + bpl, out);

		char* line = out;
		out += bpl;
		begin -= bpl;

		// BGR BMP -> RGB 内存传输
		for (; line + 3 < out; line += 3) {
			std::swap(line[0], line[2]);
		}
	}

	m_memXferYOff += rows;

	if (m_memXferYOff >= static_cast<UInt32>(std::abs(dib->biHeight))) {
		m_pendingXfers = 0;
		return { ReturnCode::XferDone, ConditionCode::Success };
	}

	return success();
}

Result SimpleDs::imageNativeXferGet(const Identity&, ImageNativeXfer& data) {
	qDebug() << "imageNativeXferGet";
	if (!m_pendingXfers) {
		return seqError();
	}

	// it does not get easier than that if we already have BMP
	data = ImageNativeXfer(bmpSize());

	std::copy(bmpBegin(), bmpEnd(), data.data<char>().data());

	m_pendingXfers = 0;
	return { ReturnCode::XferDone, ConditionCode::Success };
}

const BITMAPINFOHEADER* SimpleDs::header() const noexcept {
	//BITMAPINFOHEADER 位图文件头
	return reinterpret_cast<const BITMAPINFOHEADER*>(bmpData.data() + sizeof(BITMAPFILEHEADER));
}

UInt32 SimpleDs::bytesPerLine() const noexcept {
	auto dib = header();
	return static_cast<UInt32>(dib->biWidth * dib->biBitCount + 31) / 32 * 4;
}

UInt32 SimpleDs::bmpSize() const noexcept {
	return static_cast<UInt32>(bmpData.size()) - sizeof(BITMAPFILEHEADER);
}

const char* SimpleDs::bmpBegin() const noexcept {
	return bmpData.cbegin() + sizeof(BITMAPFILEHEADER);
}

const char* SimpleDs::bmpEnd() const noexcept {
	return bmpData.cend();
}

#if TWPP_DETAIL_OS_WIN
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		// anything you want to execute when the source is loaded
		break;
	case DLL_PROCESS_DETACH:
		// anything you want to execute when the source is unloaded
		break;
	}
	return true;
}
#endif
