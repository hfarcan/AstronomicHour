#include <stdio.h>
#include <math.h>

#define PI 3.14159265358979323846

typedef struct
{
    int SunriseHour;
    int SunriseMinute;
    int SunsetHour;
    int SunsetMinute;
} tAstroTime;

typedef struct
{
    double Latitude;            //Enlem
    double Longitude;           //Boylam
    double LocalOffset;         // UTC+3
    double Zenith;              // Resmi gün batımı için

}tGeoInfo;


double toRadians(double degrees)
{
    return degrees * (PI / 180.0);
}

double toDegrees(double radians) {
    return radians * (180.0 / PI);
}

void calculateSunriseSunset(int year, int month, int day, tGeoInfo Geo, double* sunrise, double* sunset) {
    // 1. Calculate the day of the year
    int N1 = (int)(275 * month / 9);
    int N2 = (int)((month + 9) / 12);
    int N3 = (1 + (int)((year - 4 * (int)(year / 4) + 2) / 3));
    double N = N1 - (N2 * N3) + day - 30;

    // 2. Convert the longitude to hour value and calculate an approximate time
    double lngHour = Geo.Longitude / 15.0;

    double t_rise = N + ((6 - lngHour) / 24);
    double t_set = N + ((18 - lngHour) / 24);

    // 3. Calculate the Sun's mean anomaly
    double M_rise = (0.9856 * t_rise) - 3.289;
    double M_set = (0.9856 * t_set) - 3.289;

    // 4. Calculate the Sun's true longitude
    double L_rise = M_rise + (1.916 * sin(toRadians(M_rise))) + (0.020 * sin(toRadians(2 * M_rise))) + 282.634;
    L_rise = fmod(L_rise, 360.0); // Adjust L to be within [0, 360)

    double L_set = M_set + (1.916 * sin(toRadians(M_set))) + (0.020 * sin(toRadians(2 * M_set))) + 282.634;
    L_set = fmod(L_set, 360.0); // Adjust L to be within [0, 360)

    // 5a. Calculate the Sun's right ascension
    double RA_rise = toDegrees(atan(0.91764 * tan(toRadians(L_rise))));
    RA_rise = fmod(RA_rise, 360.0); // Adjust RA to be within [0, 360)

    double RA_set = toDegrees(atan(0.91764 * tan(toRadians(L_set))));
    RA_set = fmod(RA_set, 360.0); // Adjust RA to be within [0, 360)

    // 5b. Right ascension value needs to be in the same quadrant as L
    double Lquadrant_rise = (floor(L_rise / 90.0)) * 90.0;
    double RAquadrant_rise = (floor(RA_rise / 90.0)) * 90.0;
    RA_rise = RA_rise + (Lquadrant_rise - RAquadrant_rise);

    double Lquadrant_set = (floor(L_set / 90.0)) * 90.0;
    double RAquadrant_set = (floor(RA_set / 90.0)) * 90.0;
    RA_set = RA_set + (Lquadrant_set - RAquadrant_set);

    // 5c. Right ascension value needs to be converted into hours
    RA_rise = RA_rise / 15.0;
    RA_set = RA_set / 15.0;

    // 6. Calculate the Sun's declination
    double sinDec_rise = 0.39782 * sin(toRadians(L_rise));
    double cosDec_rise = cos(asin(sinDec_rise));

    double sinDec_set = 0.39782 * sin(toRadians(L_set));
    double cosDec_set = cos(asin(sinDec_set));

    // 7a. Calculate the Sun's local hour angle
    double cosH_rise = (cos(toRadians(Geo.Zenith)) - (sinDec_rise * sin(toRadians(Geo.Latitude)))) / (cosDec_rise * cos(toRadians(Geo.Latitude)));
    double cosH_set = (cos(toRadians(Geo.Zenith)) - (sinDec_set * sin(toRadians(Geo.Latitude)))) / (cosDec_set * cos(toRadians(Geo.Latitude)));

    if (cosH_rise > 1.0 || cosH_set > 1.0) {
        *sunrise = -1; // Sun never rises
        *sunset = -1;
        return;
    }
    if (cosH_rise < -1.0 || cosH_set < -1.0) {
        *sunrise = -1; // Sun never sets
        *sunset = -1;
        return;
    }

    // 7b. Finish calculating H and convert into hours
    double H_rise = 360.0 - toDegrees(acos(cosH_rise));
    double H_set = toDegrees(acos(cosH_set));

    H_rise = H_rise / 15.0;
    H_set = H_set / 15.0;

    // 8. Calculate local mean time of rising/setting
    double T_rise = H_rise + RA_rise - (0.06571 * t_rise) - 6.622;
    double T_set = H_set + RA_set - (0.06571 * t_set) - 6.622;

    // 9. Adjust back to UTC
    double UT_rise = T_rise - lngHour;
    double UT_set = T_set - lngHour;

    UT_rise = fmod(UT_rise, 24.0); // Adjust UT to be within [0, 24)
    UT_set = fmod(UT_set, 24.0); // Adjust UT to be within [0, 24)

    // 10. Convert UT value to local time zone of latitude/longitude
    *sunrise = UT_rise + Geo.LocalOffset;
    *sunset = UT_set + Geo.LocalOffset;

    // Ensure sunrise and sunset are within [0, 24)
    if (*sunrise < 0) *sunrise += 24.0;
    if (*sunset < 0) *sunset += 24.0;
}

int main()
{
    tGeoInfo geo;  
    
    // for Istanbul
    geo.Latitude = 41.0082;
    geo.Longitude = 28.9784;
    geo.LocalOffset = 3.0;
    geo.Zenith = 90.8333;

    // Tarih bilgisi (manuel olarak ayarlanabilir)
    int year = 2025;
    int month = 12; // Eylül ayı
    int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; // Her ayın gün sayısı

    // Artık yıl kontrolü
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonth[1] = 29; // Şubat ayı artık yılda 29 gün
    }

    // CSV dosyasını aç
    FILE* file = fopen("sunrise_sunset.csv", "w");
    if (file == NULL) {
        printf("Can nt open CSV file!\n");
        return 1;
    }

    // CSV başlıklarını yaz
    fprintf(file, "Date,Sunrise,Sunset\n");

    // Belirtilen ay için tüm günleri döngüyle hesapla
    for (int day = 1; day <= daysInMonth[month - 1]; day++) {
        double sunrise, sunset;
        double zenith = 90.8333; // Resmi gün batımı için
        calculateSunriseSunset(year, month, day, geo, &sunrise, &sunset);

        // Saat ve dakikaya çevirme
        int sunriseHour = (int)sunrise;
        int sunriseMinute = (int)((sunrise - sunriseHour) * 60);

        int sunsetHour = (int)sunset;
        int sunsetMinute = (int)((sunset - sunsetHour) * 60);

        // Sonuçları CSV dosyasına yaz
        fprintf(file, "%04d-%02d-%02d,%02d:%02d,%02d:%02d\n", year, month, day, sunriseHour, sunriseMinute, sunsetHour, sunsetMinute);
    }

    // CSV dosyasını kapat
    fclose(file);
    printf("Results are writed to 'sunrise_sunset.csv' file.\n");

    return 0;
}
