#include "Strings.h"

static char const *TurkishStrs[APP_STR_COUNT] = {
	"TÜRKÇE",
	// SCREEN NAMEs //
	"BALANS",
	"STANDART ARAMA",
	"METAL ANALİZİ",
	"MİNERALLİ BÖLGE",
	"OTOMATİK ARAMA",
	"DERİNLİK HESAPLAMA",
	"SİSTEM AYARLARI",
	"PİL SEVİYESİ KRİTİK SEVİYEYE DÜŞMÜŞTÜR",
	"PIL BITMISTIR. OTOMATIK KAPANMA BASLATILIYOR...",
	// BALANS //
	"BASLAMAK ICIN ONAY BUTONUNA BASINIZ",
	"AYAR YAPILIYOR. LUTFEN BEKLEYINIZ",
	"BALANS AYARI\nYAPILDI",
	"TOPRAK MINERAL DEGERI",
	"BALANS AYARI\nYAPILAMADI",
	"LUTFEN TEKRAR\nDENEYINIZ",
	// STD SEARCH //
	"BOŞLUK",
	"MİNERAL",
	"METAL",
	// OTO SEARCH //
	"DEĞERSİZ",
	"DEĞERLİ",
	"ALTIN",
	// DEPTH CALCULATION //
	"DERINLIK ICIN CAP SECINIZ",
	"EN",
	"BOY",
	"BULUNAN OBJENİN TAHMİNi DERİNLİGİ",
	// SYS SETTINGs //
	"SES AYARI",
	"DİL SEÇİMİ",
	"YÜKSEK KONTRAST",
	"EKRAN PARLAKLIĞI",
	"DEĞERSİZ AYARI",
	"HASSASİYET",
	"FABRİKA AYARLARI",
	"AKTİF",
	"PASİF",
	"FABRIKA AYARLARINA DONMEK İSTİYOR MUSUNUZ?",
	"EVET",
	"HAYIR"
};

static char const *EnglishStrs[APP_STR_COUNT] = {
	"ENGLISH",
	// SCREEN NAMEs //
	"GROUND BALANCE",
	"STANDARD SEARCH",
	"METAL ANALYSIS",
	"MINERALIZED AREA",
	"AUTOMATIC SEARCH",
	"DEPTH CALCULATION",
	"SYSTEM SETTINGS",
	"BATTERY LEVEL IS CRITICAL",
	"BATTERY IS EMPTY. AUTOMATIC POWER OFF IS ACTIVATED",
	// BALANS //
	"PRESS CONFIRM TO START",
	"PROCESSING, PLEASE WAIT",
	"GROUND BALANCE\nCOMPLETED",
	"GROUND ID",
	"GROUND BALANCE\xnFAILED",
	"PLEASE TRY AGAIN",
	// STD SEARCH //
	"CAVITY",
	"MINERAL",
	"METAL",
	// OTO SEARCH //
	"FERROS",
	"NONFERROS",
	"GOLD",
	// DEPTH CALCULATION //
	"SELECT TARGET SIZE FOR CALCULATION",
	"TARGET WIDTH",
	"TARGET HEIGHT",
	"ESTIMATED TARGET DEPTH",
	// SYS SETTINGs //
	"VOLUME",
	"LANGUAGE",
	"HIGH CONTRAST MODE",
	"LCD BRIGHTNESS",
	"FERROS ELIMINATION",
	"SENSITIVITY",
	"FACTORY SETTINGS",
	"ENABLE",
	"DISABLE",
	"SURE TO RESET TO FACTORY SETTINGS",
	"YES",
	"NO"
};

static char const *ArabicStrs[APP_STR_COUNT] = {
	"عربي",
	// SCREEN NAMEs //
	"موازنة",
	"بحث قياسي",
	"تحليل المعدن",
	"منطقة معدنية",
	"بحث تلقائي",
	"حساب العمق",
	"عيارات المنظومة",
	"مستوى البطارية منخفض الى المستوى الحرج",
	"البطارية منتهية.. يتم البدء بالاغلاق التلقائي"
	// BALANS //
	"للبدء اضغط على زر الموافقة",
	"جاري تنظيم العيارات. يرجى الانتظار",
	"تمت الموازنة. قيمة المعادن في التربة : ",
	"GROUND ID",
	"لم تتم الموازنة. يرجى اعادة المحاولة",
	"PLEASE TRY AGAIN",
	// STD SEARCH //
	"فراغ",
	"معدني",
	"معدن",
	// OTO SEARCH //
	"بدون قيمة",
	"ذو قيمة",
	"ذهب",
	// DEPTH CALCULATION //
	"لتحديد العمق يرجى اختيار القطر",
	"العرض",
	"الطول",
	"العمق التخميني للجسم الموجود : ",
	// SYS SETTINGs //
	"ضبط الصوت",
	"ضبط اللغة",
	"تباين عالي",
	"لمعان الشاشة",
	"عيار بدون قيمة",
	"الحساسية",
	"عيارات المصنع",
	"فعال",
	"غير فعال",
	"هل ترغب بالعودة الى عيارات المصنع؟ ",
	"نعم",
	"كلا"
};


static char const *GermanStrs[APP_STR_COUNT] = {
	"GERMAN",
	// SCREEN NAMEs //
	"GRUNDBALANZ",
	"STANDARDSUCHE",
	"METALLANALYSE",
	"MINERALISIERUNG D-BEREICH",
	"AUTOMATISCHE SUCHE",
	"TIEFENBERECHNUNG",
	"SYSTEME INSTELLUNGEN",
	"BATTERIE STAND IST KRITISCH",
	"BATTERIE IST LEER. AUTOMATISCHE AUSSCHALTUNG AKTIVIERT",
	// BALANCE //
	"ZUM STARTEN AUF BESTAETIGUNG DRÜCKEN",
	"WIRD VERARBEITET. BITTE WARTEN.",
	"GRUNDBALANZ ABGESCHLOSSEN.",
	"GRUND ID",
	"GRUNDBALANZ GESCHEITERT.",
	"BITTE ERNEUT VERSUCHEN",
	// STD SEARCH //
	"KAVITAET",
	"MINERAL",
	"METALL",
	// OTO SEARCH //
	"FERROS",
	"NICHT FERROS",
	"GOLD",
	// DEPTH CALCULATION //
	"FÜR DIE TIEFE EIN ZIELAUSMASS AUSSUCHEN",
	"ZIELBREITE",
	"ZIELHÖHE",
	"EINGESCHAETZTE ZIELTIEFE",
	// SYS SETTINGs //
	"LAUTSTAERKENEINSTELLUNG",
	"SPRACHE",
	"HÖHE KONTRAST MODUS",
	"LCD HELLIGKEIT",
	"FERROS ELIMINATION",
	"SENSIBILITAET",
	"WERKEINSTELLUNGEN",
	"AKTIV",
	"NICHT AKTIV",
	"SIND SIE SICHER, DASS SIE AUF WERKEINSTELLUNGEN ZURÜCKSETZEN WOLLEN?",
	"JA",
	"NEIN"
};

char const *SpanishStrs[APP_STR_COUNT] = {
	"ESPAÑOL",
	// SCREEN NAMEs //
	"BALANCE DE SUELO",
	"BÚSQUEDA ESTÁNDAR",
	"ANÁLISIS DE METAL",
	"ÁREA MINERALIZADA",
	"BÚSQUEDA AUTOMÁTICA",
	"CÁLCULO DE PROFUNDIDAD",
	"AJUSTES DEL SISTEMA",
	"LA BATERÍA HA ALCANZADO UN NIVEL CRÍTICO.",
	"LA BATERÍA ESTÁ VACÍA... SE ACTIVA EL AUTOAPAGADO",
	// BALANS //
	"PULSE CONFIRMAR PARA INICIAR",
	"PROCESANDO. POR FAVOR ESPERE",
	"BALANCE DE SUELO COMPLETADO",
	"VALOR DEL MINERAL DEL SUELO",
	"BALANCE DE SUELO FALLADO",
	"INTÉNTELO DE NUEVO",
	// STD SEARCH //
	"CAVIDAD",
	"MINERAL",
	"METAL",
	// OTO SEARCH //
	"FERROSO",
	"NO FERROSO",
	"ORO",
	// DEPTH CALCULATION //
	"SELECCIONE DIÁMETRO PARA LA PROFUNDIDAD",
	"ANCHO",
	"LARGO",
	"PROFUNDIDAD ESTIMADA DEL OBJETO DETECTADO",
	// SYS SETTINGs //
	"VOLUMEN",
	"IDIOMA",
	"MODO DE ALTO CONTRASTE",
	"BRILLO DE LA PANTALLA",
	"AJUSTE DE FERROSO",
	"SENSIBILIDAD",
	"CONFIGURACIÓN DE FÁBRICA",
	"ACTIVAR",
	"DESACTIVAR",
	"¿DESEA REGRESAR A LA CONFIGURACIÓN DE FÁBRICA?",
	"SÍ",
	"NO"
};

static char const *PersianStrs[APP_STR_COUNT] = {
	"فارسی",
	// SCREEN NAMEs //
	"بالانس",
	"جستجوی استاندارد",
	"آنالیز فلز",
	"منطقه مینرالیزه",
	"جستجوی اتوماتیک",
	"محاسبه عمق",
	"تنظیمات سیستم",
	"سطح باتری به سطح  بحرانی رسیده است.",
	"باتری تمام شده است ...اتوماتیک بسته می شود.",
	// BALANS //
	"برای شروع دکمه تائید را فشار دهید.",
	"در حال تنظیم می باشد. لطفا صبر کنید.",
	"بالانس انجام شد.ارزش معدنی خاک",
	"GROUND ID",
	"بالانس انجام نشد. لطفا مجددا امتحان نمائید.",
	"PLEASE TRY AGAIN",
	// STD SEARCH //
	"فضای خالی",
	"معدنی",
	"فلز",
	// OTO SEARCH //
	"بی ارزش",
	"با ارزش",
	"طلا",
	// DEPTH CALCULATION //
	"برای عمق، قطر انتخاب نمائید.",
	"عرض",
	"طول",
	"عمق تخمینی مورد بدست آمده:",
	// SYS SETTINGs //
	"تنظیمات صدا",
	"تنظیمات زبان",
	"کنتراست بالا",
	"روشنایی صفحه نمایش",
	"تنظیمات بی ارزش",
	"حساسیت",
	"تنظیمات کارخانه",
	"فعال",
	"غیرفعال",
	"آیا میخواهید به تنظیمات کارخانه بازگردید؟",
	"بله",
	"نه"
};

static char const *RussianStrs[APP_STR_COUNT] = {
	"РУССКИЙ",
	// SCREEN NAMEs //
	"БАЛАНС",
	"СТАНДАРТНЫЙ ПОИСК",
	"АНАЛИЗ МЕТАЛЛА",
	"МИНЕРАЛЬНАЯ ЗОНА",
	"АВТОМАТИЧЕСКИЙ ПОИСК",
	"РАСЧЕТ ГЛУБИНЫ",
	"НАСТРОЙКИ СИСТЕМЫ",
	"ЗАРЯДКА БАТАРЕИ НА КРИТИЧЕСКОМ УРОВНЕ",
	"БАТАРЕЯ РАЗРЯДИЛАСЬ. ВЫПОЛНЯЕТСЯ АВТОМАТИЧЕСКОЕ ВЫКЛЮЧЕНИЕ",
	// BALANS //
	"НАЖМИТЕ НА КНОПКУ ПОДТВЕРЖДЕНИЯ ДЛЯ НАЧАЛА ОПЕРАЦИИ",
	"ВЫПОЛНЯЕТСЯ НАСТРОЙКА. ПОЖАЛУЙСТА ПОДОЖДИТЕ",
	"БАЛАНСИРОВКА ЗАВЕРШЕНА",
	"МИНЕРАЛЬНОЕ СОДЕРЖАНИЕ ГРУНТА",
	"БАЛАНСИРОВКА НЕ ВЫПОЛНЕНА",
	"ПОПРОБУЙТЕ ЕЩЕ РАЗ",
	// STD SEARCH //
	"ПУСТОТА",
	"МИНЕРАЛ",
	"МЕТАЛЛ",
	// OTO SEARCH //
	"НЕ ДРАГОЦЕННЫЙ",
	"ДРАГОЦЕННЫЙ",
	"ЗОЛОТО",
	// DEPTH CALCULATION //
	"ВЫБЕРИТЕ ДИАМЕТР ДЛЯ ГЛУБИНЫ",
	"ШИРИНА",
	"ДЛИНА",
	"ПРИБЛИЗИТЕЛЬНАЯ ГЛУБИНА НАЙДЕННОГО ОБЪЕКТА",
	// SYS SETTINGs //
	"НАСТРОЙКА ЗВУКА",
	"НАСТРОЙКА ЯЗЫКА",
	"ВЫСОКИЙ КОНТРАСТ",
	"ЯРКОСТЬ ЭКРАНА",
	"НАСТРОЙКА НЕДРАГОЦЕННЫЙ",
	"ТОЧНОСТЬ",
	"ЗАВОДСКИЕ НАСТРОЙКИ",
	"АКТИВНЫЙ",
	"НЕАКТИВНЫЙ",
	"ВЫ ХОТИТЕ ВЕРНУТЬСЯ К ЗАВОДСКИМ НАСТРОЙКАМ?",
	"ДА",
	"НЕТ"
};

static char const *FrenchStrs[APP_STR_COUNT] = {
	"FRANÇAIS",
	// SCREEN NAMEs //
	"EQUILIBRE DU SOL",
	"RECHERCHE STANDARD",
	"ANALYSE DE METAL",
	"ZONE MINERALISEE",
	"RECHERCHE AUTOMATIQUE",
	"CALCUL DE PROFONDEUR",
	"REGLAGES DU SYSTEME",
	"LE NIVEAU DE LA BATTERIE EST DESCENDU JUSQU’AU NIVEAU CRITIQUE",
	"LA BATTERIE EST VIDE… L’EXTINCTION AUTOMATIQUE EST ACTIVEE",
	// BALANS //
	"PRESSER SUR LE BOUTON DE CONFIRMATION POUR COMMENCER",
	"REGLAGE EN COURS. VEUILLEZ PATIENTER ",
	"L’EQUILIBRAGE DU SOL EST TERMINE",
	"VALEUR MINERALE DU SOL",
	"L’EQUILIBRAGE DU SOL A ECHOUE",
	"VEUILLEZ RECOMMENCER",
	// STD SEARCH //
	"CAVITE",
	"MINERAL",
	"METAL",
	// OTO SEARCH //
	"SANS VALEUR",
	"DE VALEUR",
	"OR",
	// DEPTH CALCULATION //
	"VEUILLEZ CHOISIR UN DIAMETRE POUR LA PROFONDEUR",
	"LARGEUR",
	"LONGUEUR",
	"VALEUR ESTIME DE L’OBJET DECOUVERT",
	// SYS SETTINGs //
	"VOLUME",
	"LANGAGE",
	"CONTRASTE ELEVE",
	"LUMINOSITE DE L’ECRAN",
	"REGLAGE SANS VALEUR",
	"SENSIBILITE",
	"REGLAGE D’USINE",
	"ACTIF",
	"PASSIF",
	"DESIREZ-VOUS RETOURNER AUX REGLAGES D’USINE",
	"OUI",
	"NON"
};

char const **AllLangStrs[LANG_COUNT] = 
{
	TurkishStrs, EnglishStrs, ArabicStrs, \
	GermanStrs, SpanishStrs, PersianStrs, RussianStrs, FrenchStrs
};
