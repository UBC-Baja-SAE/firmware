import re
import struct

def convert_hex_to_decimal(data):
    """
    Parses a block of text, extracts the 8-byte hexadecimal sequence from each 
    line, and converts it to a standard Python double-precision float 
    (IEEE 754 64-bit) using little-endian byte order.

    Args:
        data (str): The raw input text containing multiple lines of data.

    Returns:
        list: A list of dictionaries, each containing the original line 
              and the calculated decimal value.
    """
    
    # Regex to capture the 8 space-separated hex bytes at the end of the line
    # (e.g., "07 BC AF D5 37 F9 DF 3F")
    # This pattern looks for 8 pairs of hex characters (A-F, 0-9) separated by a space.
    hex_pattern = re.compile(r'([0-9A-F]{2}\s+){7}[0-9A-F]{2}')
    
    results = []
    
    # Process each line
    for line in data.strip().split('\n'):
        line = line.strip()
        
        # 1. Find the hex data using the compiled pattern
        match = hex_pattern.search(line)
        
        if match:
            # 2. Extract the raw hex string (e.g., '07 BC AF D5 37 F9 DF 3F')
            hex_data_spaced = match.group(0).strip()
            
            # 3. Concatenate the hex bytes into a single 16-character string
            # (e.g., '07BCAFD537F9DF3F')
            hex_data_raw = hex_data_spaced.replace(' ', '')
            
            try:
                # 4. Convert the hex string to a binary bytes object
                byte_data = bytes.fromhex(hex_data_raw)
                
                # 5. Unpack the bytes as a little-endian ('<') double-precision float ('d')
                # The little-endian format matches the order in which the bytes were logged.
                decimal_value = struct.unpack('<d', byte_data)[0]
                
                results.append({
                    "original_line": line,
                    "hex_bytes_le": hex_data_spaced,
                    "decimal_value": decimal_value
                })
            except Exception as e:
                print(f"Error processing line '{line}': {e}")
                
    return results

# --- Input Data ---
input_data = """
     2)         5.4  Rx         0001  8  D4 1A DC 88 4A 7C 41 40 
     6)        21.3  Rx         0001  8  6E 6B 08 30 90 3B 43 40 
    10)        37.3  Rx         0001  8  66 04 C5 43 67 F6 44 40 
    14)        53.3  Rx         0001  8  30 28 1F 0A 63 A8 46 40 
    18)        69.3  Rx         0001  8  8A 7C 99 70 2D 4D 48 40 
    22)        85.3  Rx         0001  8  E2 ED 96 23 92 E0 49 40 
    26)       101.3  Rx         0001  8  38 79 74 4F 89 5E 4B 40 
    30)       117.3  Rx         0001  8  A8 CB CF EF 41 C3 4C 40 
    34)       133.4  Rx         0001  8  79 CC 9B 92 2B 0B 4E 40 
    38)       149.4  Rx         0001  8  FF C0 0A 76 FF 32 4F 40 
    42)       165.4  Rx         0001  8  18 19 FE 74 E4 1B 50 40 
    46)       181.4  Rx         0001  8  57 6D BF 6F 76 8B 50 40 
    50)       197.4  Rx         0001  8  DD EB 87 49 18 E7 50 40 
    54)       213.4  Rx         0001  8  62 61 45 A0 DF 2D 51 40 
    58)       229.4  Rx         0001  8  C9 2F 1A 69 17 5F 51 40 
    62)       245.4  Rx         0001  8  E8 70 72 BF 41 7A 51 40 
    66)       261.4  Rx         0001  8  01 64 08 27 19 7F 51 40 
    70)       277.4  Rx         0001  8  78 78 A0 3D 91 6D 51 40 
    74)       293.4  Rx         0001  8  B4 56 B6 DA D6 45 51 40 
    78)       309.4  Rx         0001  8  61 E1 C9 9C 4F 08 51 40 
    82)       325.4  Rx         0001  8  A8 91 71 E5 98 B5 50 40 
    86)       341.4  Rx         0001  8  11 0B CB 46 86 4E 50 40 
    90)       357.4  Rx         0001  8  75 32 9F CC 3E A8 4F 40 
    94)       373.4  Rx         0001  8  F1 A9 E9 B4 3A 8F 4E 40 
    98)       389.4  Rx         0001  8  1C CB AC 13 CF 54 4D 40 
   102)       405.4  Rx         0001  8  52 9A D2 27 20 FC 4B 40 
   106)       421.4  Rx         0001  8  F7 65 D0 98 9F 88 4A 40 
   110)       437.4  Rx         0001  8  30 43 7F A7 03 FE 48 40 
   114)       453.4  Rx         0001  8  E0 D5 7C AF 3D 60 47 40 
   118)       469.4  Rx         0001  8  54 99 5D 11 70 B3 45 40 
   122)       485.4  Rx         0001  8  DC 92 7C 9F E3 FB 43 40 
   126)       501.4  Rx         0001  8  80 0F 75 A8 FC 3D 42 40 
   130)       517.4  Rx         0001  8  A5 95 4E BB 2F 7E 40 40 
   134)       533.4  Rx         0001  8  D7 0E 2E 84 EC 81 3D 40 
   138)       549.4  Rx         0001  8  E8 8D 22 20 86 15 3A 40 
   142)       565.4  Rx         0001  8  AB E4 7C 01 EE BF 36 40 
   146)       581.4  Rx         0001  8  1D 62 B9 89 AB 89 33 40 
   150)       597.4  Rx         0001  8  98 FB 01 F4 F5 7A 30 40 
   154)       613.4  Rx         0001  8  7C 0B CA A2 3E 37 2B 40 
   158)       629.4  Rx         0001  8  84 B4 9F 0F 01 E6 25 40 
   162)       645.4  Rx         0001  8  06 9E F0 0D CD 0F 21 40 
   166)       661.4  Rx         0001  8  39 7A 1C 8D 03 82 19 40 
   170)       677.4  Rx         0001  8  AE A0 06 0D 48 09 12 40 
   174)       693.4  Rx         0001  8  F5 24 5F 3F 08 91 07 40 
   178)       709.4  Rx         0001  8  16 E5 E5 42 D9 3E FB 3F 
   182)       725.4  Rx         0001  8  FB 20 57 50 B2 5C E9 3F 
   186)       741.4  Rx         0001  8  48 F9 0E 88 89 AE CC 3F 
   190)       757.4  Rx         0001  8  00 49 C5 97 E0 0A 69 3F 
   194)       773.4  Rx         0001  8  34 17 23 34 0C DC C0 3F 
   198)       789.4  Rx         0001  8  29 B7 59 FD 02 7B E3 3F 
   202)       805.4  Rx         0001  8  FA AB 32 46 FF DE F6 3F 
   206)       821.4  Rx         0001  8  1E 9B AB 4E 32 AF 04 40 
   210)       837.4  Rx         0001  8  41 87 17 40 18 43 10 40 
   214)       853.4  Rx         0001  8  F3 7E 70 AB 18 6B 17 40 
   218)       869.4  Rx         0001  8  A0 CE F7 64 4C BD 1F 40 
   222)       885.4  Rx         0001  8  56 76 39 53 35 92 24 40 
   226)       901.4  Rx         0001  8  6A D3 89 2C 33 C4 29 40 
   230)       917.4  Rx         0001  8  3F 48 3E CD 55 67 2F 40 
   234)       933.4  Rx         0001  8  B3 FF 0B EE 98 B6 32 40 
   238)       949.4  Rx         0001  8  64 79 E1 BC 2F E3 35 40 
   242)       965.4  Rx         0001  8  B4 7A 5E D4 50 31 39 40 
   246)       981.4  Rx         0001  8  00 AA 2E EB 87 98 3C 40 
   250)       997.4  Rx         0001  8  CC CA AF 46 10 08 40 40 
   254)      1013.3  Rx         0001  8  72 7D 3B 30 9E C7 41 40 
   258)      1029.3  Rx         0001  8  1E 0C 5B E9 74 86 43 40 
   262)      1045.3  Rx         0001  8  3F EA C1 7D 1D 40 45 40 
   266)      1061.3  Rx         0001  8  AE E6 3C 39 2E F0 46 40 
   270)      1077.3  Rx         0001  8  C5 28 54 F1 55 92 48 40 
   274)      1093.3  Rx         0001  8  34 57 29 10 67 22 4A 40 
   278)      1109.3  Rx         0001  8  DA 27 53 44 62 9C 4B 40 
   282)      1125.3  Rx         0001  8  98 62 60 BA 80 FC 4C 40 
   286)      1141.4  Rx         0001  8  AE 17 DB C5 3D 3F 4E 40 
   290)      1157.4  Rx         0001  8  3B 66 16 E1 5F 61 4F 40 
   294)      1173.4  Rx         0001  8  E2 FF 60 76 00 30 50 40 
   298)      1189.4  Rx         0001  8  85 23 15 CD 4A 9C 50 40 
   302)      1205.3  Rx         0001  8  8A C8 96 F6 79 F4 50 40 
   306)      1221.3  Rx         0001  8  D2 9D A0 62 AC 37 51 40 
   310)      1237.3  Rx         0001  8  5E 06 AF 2F 36 65 51 40 
   314)      1253.3  Rx         0001  8  E3 22 A6 E2 A2 7C 51 40 
   318)      1269.3  Rx         0001  8  37 25 C3 90 B6 7D 51 40 
   322)      1285.3  Rx         0001  8  7A D4 DE 78 6E 68 51 40 
   326)      1301.3  Rx         0001  8  54 3D 79 0A 01 3D 51 40 
   330)      1317.3  Rx         0001  8  A0 88 7C 5A DD FB 50 40 
   334)      1333.3  Rx         0001  8  38 20 1B 07 AA A5 50 40 
   338)      1349.3  Rx         0001  8  8C E7 9F 8D 43 3B 50 40 
   342)      1365.3  Rx         0001  8  3F 52 E5 2C 74 7B 4F 40 
   346)      1381.3  Rx         0001  8  46 A1 E6 79 9D 5C 4E 40 
   350)      1397.4  Rx         0001  8  B9 2C 41 B4 E0 1C 4D 40 
   354)      1413.4  Rx         0001  8  5D 05 7E B4 6F BF 4B 40 
   358)      1429.4  Rx         0001  8  D6 E9 C0 4D C8 47 4A 40 
   362)      1445.4  Rx         0001  8  6A 43 7F 5F AB B9 48 40 
   366)      1461.4  Rx         0001  8  8F 15 B7 3B 13 19 47 40 
   370)      1477.4  Rx         0001  8  A7 90 34 7A 29 6A 45 40 
   374)      1493.4  Rx         0001  8  87 D4 EC 52 3C B1 43 40 
   378)      1509.4  Rx         0001  8  D7 B9 A9 9A B3 F2 41 40 
   382)      1525.4  Rx         0001  8  58 1A 32 7E 05 33 40 40 
   386)      1541.4  Rx         0001  8  6A 47 83 31 56 ED 3C 40 
   390)      1557.4  Rx         0001  8  DB 31 02 06 2A 84 39 40 
   394)      1573.4  Rx         0001  8  0E 29 86 EF 3F 33 36 40 
   398)      1589.4  Rx         0001  8  10 60 23 57 13 03 33 40 
   402)      1605.4  Rx         0001  8  FE E7 5D CE 97 F7 2F 40 
   406)      1621.4  Rx         0001  8  21 A1 8C 5E 51 4A 2A 40 
   410)      1637.4  Rx         0001  8  08 FA CD A7 D8 0C 25 40 
   414)      1653.4  Rx         0001  8  D3 B9 8A F8 94 4C 20 40 
   418)      1669.4  Rx         0001  8  04 A1 8B B7 5A 2B 18 40 
   422)      1685.4  Rx         0001  8  1F 56 F3 06 D3 E5 10 40 
   426)      1701.4  Rx         0001  8  08 FC A3 D6 58 B6 05 40 
   430)      1717.4  Rx         0001  8  9F F5 56 15 6C 6B F8 3F 
   434)      1733.4  Rx         0001  8  6A D6 AB 18 30 88 E5 3F 
   438)      1749.4  Rx         0001  8  04 23 8F 42 11 CD C4 3F 
   442)      1765.4  Rx         0001  8  00 C0 C8 7E 06 B1 2D 3F 
   446)      1781.4  Rx         0001  8  E4 B9 3B 34 F3 04 C8 3F 
   450)      1797.4  Rx         0001  8  54 BA 08 39 12 22 E7 3F 
   454)      1813.4  Rx         0001  8  60 D2 D1 C0 45 9C F9 3F 
   458)      1829.4  Rx         0001  8  F9 B9 E8 17 34 7F 06 40 
   462)      1845.4  Rx         0001  8  AD 1A 88 FB 76 61 11 40 
   466)      1861.4  Rx         0001  8  93 BB 91 BE F8 BC 18 40 
   470)      1877.4  Rx         0001  8  CC 68 26 C9 A6 9F 20 40 
   474)      1893.4  Rx         0001  8  62 F4 5F CA 58 69 25 40 
   478)      1909.4  Rx         0001  8  8F CD 6F 38 53 AF 2A 40 
   482)      1925.4  Rx         0001  8  BC 0B 4F 81 0C 32 30 40 
   486)      1941.4  Rx         0001  8  D0 3B 3E D9 88 3C 33 40 
   490)      1957.4  Rx         0001  8  BB 26 A3 60 57 6F 36 40 
   494)      1973.4  Rx         0001  8  89 7A 53 B1 49 C2 39 40 
   498)      1989.4  Rx         0001  8  10 D5 8E 2F DF 2C 3D 40 
   502)      2005.4  Rx         0001  8  75 43 BC 64 2D 53 40 40 
   506)      2021.3  Rx         0001  8  59 25 9C A8 EC 12 42 40 
   510)      2037.3  Rx         0001  8  5C BF 4A 1C 34 D1 43 40 
   514)      2053.2  Rx         0001  8  B5 8C F3 39 8E 89 45 40 
   518)      2069.3  Rx         0001  8  E1 FA E6 A4 94 37 47 40 
   522)      2085.3  Rx         0001  8  0A 1C B0 6A FB D6 48 40 
   526)      2101.2  Rx         0001  8  D2 76 99 00 9C 63 4A 40 
   530)      2117.2  Rx         0001  8  E8 57 84 E1 7F D9 4B 40 
   534)      2133.2  Rx         0001  8  D3 C4 EA B0 EA 34 4D 40 
   538)      2149.3  Rx         0001  8  91 60 1E C9 63 72 4E 40 
   542)      2165.3  Rx         0001  8  F3 15 53 1C BF 8E 4F 40 
   546)      2181.3  Rx         0001  8  14 B2 DF A8 92 43 50 40 
   550)      2197.3  Rx         0001  8  01 A5 08 85 8D AC 50 40 
   554)      2213.3  Rx         0001  8  6A 81 50 9C 43 01 51 40 
   558)      2229.3  Rx         0001  8  19 45 92 40 DC 40 51 40 
   562)      2245.3  Rx         0001  8  F0 13 15 C6 B4 6A 51 40 
   566)      2261.3  Rx         0001  8  2E 56 A3 23 62 7E 51 40 
   570)      2277.3  Rx         0001  8  43 61 53 04 B2 7B 51 40 
   574)      2293.3  Rx         0001  8  B4 5D 45 48 AB 62 51 40 
   578)      2309.3  Rx         0001  8  20 1C 0D F3 8D 33 51 40 
   582)      2325.3  Rx         0001  8  57 D5 F4 87 D2 EE 50 40 
   586)      2341.3  Rx         0001  8  4B A8 BB D5 28 95 50 40 
   590)      2357.3  Rx         0001  8  1E 50 E4 34 76 27 50 40 
   594)      2373.3  Rx         0001  8  A7 A6 24 7A A6 4D 4F 40 
   598)      2389.3  Rx         0001  8  2E 6B A4 EE 11 29 4E 40 
   602)      2405.3  Rx         0001  8  F4 FF F0 28 1B E4 4C 40 
   606)      2421.3  Rx         0001  8  EF CB 00 60 01 82 4B 40 
   610)      2437.3  Rx         0001  8  2D 7B F4 51 4E 06 4A 40 
   614)      2453.3  Rx         0001  8  DD F5 51 37 CD 74 48 40 
   618)      2469.3  Rx         0001  8  2F 5B C3 0E 81 D1 46 40 
   622)      2485.3  Rx         0001  8  A9 64 2C 5A 9A 20 45 40 
   626)      2501.3  Rx         0001  8  A5 7F 55 67 6C 66 43 40 
   630)      2517.3  Rx         0001  8  16 B1 95 3F 62 A7 41 40 
   634)      2533.3  Rx         0001  8  74 02 90 B5 E6 CF 3F 40 
   638)      2549.3  Rx         0001  8  9D E6 E8 64 30 59 3C 40 
   642)      2565.3  Rx         0001  8  3A C7 CB A4 7D F3 38 40 
   646)      2581.3  Rx         0001  8  20 78 D9 07 7F A7 35 40 
   650)      2597.3  Rx         0001  8  67 68 94 61 A3 7D 32 40 
   654)      2613.3  Rx         0001  8  F2 CE C5 68 04 FC 2E 40 
   658)      2629.3  Rx         0001  8  F4 68 D3 FD 8D 60 29 40 
   662)      2645.4  Rx         0001  8  1C 28 60 3B 3B 37 24 40 
   666)      2661.4  Rx         0001  8  F4 CE EF CC 7F 1A 1F 40 
   670)      2677.4  Rx         0001  8  41 47 46 16 14 DD 16 40 
   674)      2693.4  Rx         0001  8  21 61 38 E2 8D 96 0F 40 
   678)      2709.4  Rx         0001  8  DD 5D 95 31 5B EE 03 40 
   682)      2725.3  Rx         0001  8  FA 26 D6 92 C2 BE F5 3F 
   686)      2741.3  Rx         0001  8  19 35 AC 2F 2F 03 E2 3F 
   690)      2757.3  Rx         0001  8  48 A4 7A 4B B1 5C BC 3F 
   694)      2773.3  Rx         0001  8  80 26 23 66 B1 DD 7D 3F 
   698)      2789.3  Rx         0001  8  48 C9 B8 1D 2F 38 D0 3F 
   702)      2805.3  Rx         0001  8  FA D1 BC 18 85 18 EB 3F 
   706)      2821.3  Rx         0001  8  CE AC FB D5 39 80 FC 3F 
   710)      2837.3  Rx         0001  8  E8 AE 30 1B D9 61 08 40 
   714)      2853.3  Rx         0001  8  F0 B1 4B B4 B5 88 12 40 
   718)      2869.3  Rx         0001  8  2E E8 E0 7B 30 17 1A 40 
   722)      2885.3  Rx         0001  8  31 B3 F5 5F 84 64 21 40 
   726)      2901.3  Rx         0001  8  0F 96 FD 8A 00 44 26 40 
   730)      2917.3  Rx         0001  8  13 B7 4E D8 95 9D 2B 40 
   734)      2933.3  Rx         0001  8  B4 A2 CA 88 CA B1 30 40 
   738)      2949.3  Rx         0001  8  17 CC D1 D8 9C C3 33 40 
   742)      2965.3  Rx         0001  8  06 F8 41 D5 67 FC 36 40 
   746)      2981.3  Rx         0001  8  2A 53 1E C8 ED 53 3A 40 
   750)      2997.3  Rx         0001  8  95 6A C4 60 A2 C1 3D 40 
   754)      3013.3  Rx         0001  8  F7 C7 AB 48 60 9E 40 40 
   758)      3029.2  Rx         0001  8  45 EC A6 7E 30 5E 42 40 
   762)      3045.2  Rx         0001  8  4A 04 DF 5F C8 1B 44 40 
   766)      3061.2  Rx         0001  8  9C A2 96 27 B4 D2 45 40 
   770)      3077.2  Rx         0001  8  ED F9 27 22 91 7E 47 40 
   774)      3093.2  Rx         0001  8  B9 BD BD E4 18 1B 49 40 
   778)      3109.2  Rx         0001  8  DA F7 B2 3C 2C A4 4A 40 
   782)      3125.2  Rx         0001  8  7C 65 A1 BA DD 15 4C 40 
   786)      3141.2  Rx         0001  8  7A 87 26 BE 7B 6C 4D 40 
   790)      3157.3  Rx         0001  8  2B 27 AD E8 99 A4 4E 40 
   794)      3173.3  Rx         0001  8  F4 5F 10 DF 19 BB 4F 40 
   798)      3189.3  Rx         0001  8  04 2F D9 A1 99 56 50 40 
   802)      3205.3  Rx         0001  8  12 B7 4F 6A 3D BC 50 40 
   806)      3221.3  Rx         0001  8  9A 4A C4 4D 74 0D 51 40 
   810)      3237.3  Rx         0001  8  33 0A E1 8F 6E 49 51 40 
   814)      3253.3  Rx         0001  8  53 F3 7D C6 92 6F 51 40 
   818)      3269.3  Rx         0001  8  9C F5 0A 62 7F 7F 51 40 
   822)      3285.3  Rx         0001  8  37 20 1C A7 0B 79 51 40 
   826)      3301.3  Rx         0001  8  07 98 99 16 48 5C 51 40 
   830)      3317.3  Rx         0001  8  4A D7 88 43 7E 29 51 40 
   834)      3333.3  Rx         0001  8  04 31 DB 16 30 E1 50 40 
   838)      3349.3  Rx         0001  8  B6 F7 22 83 16 84 50 40 
   842)      3365.3  Rx         0001  8  21 3D 81 AB 1F 13 50 40 
   846)      3381.3  Rx         0001  8  B3 71 0C 05 D9 1E 4F 40 
   850)      3397.3  Rx         0001  8  6E C8 34 CE 9B F4 4D 40 
   854)      3413.3  Rx         0001  8  7C 93 A5 8D 82 AA 4C 40 
   858)      3429.3  Rx         0001  8  45 24 99 9C D9 43 4B 40 
   862)      3445.3  Rx         0001  8  4E BE 9E 62 36 C4 49 40 
   866)      3461.3  Rx         0001  8  62 37 01 2B 6E 2F 48 40 
   870)      3477.3  Rx         0001  8  28 CC C1 56 8C 89 46 40 
   874)      3493.3  Rx         0001  8  AD 75 3B 04 C8 D6 44 40 
   878)      3509.3  Rx         0001  8  85 E8 E4 46 79 1B 43 40 
   882)      3525.3  Rx         0001  8  C4 92 C5 0A 0E 5C 41 40 
   886)      3541.3  Rx         0001  8  80 1F 13 80 FD 39 3F 40 
   890)      3557.3  Rx         0001  8  F1 EC 5E D7 85 C5 3B 40 
   894)      3573.3  Rx         0001  8  98 6B 1F 75 8B 63 38 40 
   898)      3589.3  Rx         0001  8  46 5D EE 67 B5 1C 35 40 
   902)      3605.3  Rx         0001  8  06 F1 7B 51 65 F9 31 40 
   906)      3621.3  Rx         0001  8  F7 19 A2 EC 43 03 2E 40 
   910)      3637.3  Rx         0001  8  B5 14 F9 6B 05 7A 28 40 
   914)      3653.3  Rx         0001  8  AF 2C 5E 40 38 65 23 40 
   918)      3669.3  Rx         0001  8  B1 65 C2 61 B7 A3 1D 40 
   922)      3685.3  Rx         0001  8  F4 A6 08 DB 47 97 15 40 
   926)      3701.3  Rx         0001  8  C2 96 89 7D 70 73 0D 40 
   930)      3717.3  Rx         0001  8  0E 0B 2B 51 30 39 02 40 
   934)      3733.3  Rx         0001  8  4E 60 5D 49 0E 39 F3 3F 
   938)      3749.3  Rx         0001  8  52 81 BD 97 E1 9B DD 3F 
   942)      3765.3  Rx         0001  8  60 C2 89 B5 B4 A5 B1 3F 
   946)      3781.3  Rx         0001  8  00 5C 7F EC F4 D4 98 3F 
   950)      3797.3  Rx         0001  8  0A 46 37 A4 D8 0E D5 3F 
   954)      3813.3  Rx         0001  8  5B 65 EE 2F 12 5E EF 3F 
   958)      3829.3  Rx         0001  8  A4 E8 3A F7 A5 8A FF 3F 
   962)      3845.3  Rx         0001  8  FA D1 AE 69 FE 56 0A 40 
   966)      3861.3  Rx         0001  8  56 43 D8 0B BF B8 13 40 
   970)      3877.3  Rx         0001  8  47 2F 5D D4 A6 79 1B 40 
   974)      3893.3  Rx         0001  8  F5 52 3A B7 30 2D 22 40 
   978)      3909.3  Rx         0001  8  A7 2A A6 C1 1C 22 27 40 
   982)      3925.3  Rx         0001  8  85 A2 78 CD E9 8E 2C 40 
   986)      3941.3  Rx         0001  8  06 29 28 BE DB 32 31 40 
   990)      3957.3  Rx         0001  8  21 67 F0 25 CB 4B 34 40 
   994)      3973.3  Rx         0001  8  62 07 FD E4 56 8A 37 40 
   998)      3989.3  Rx         0001  8  B6 24 31 8E 32 E6 3A 40 
  1002)      4005.3  Rx         0001  8  E4 1B 6B BA C6 56 3E 40 
  1006)      4021.3  Rx         0001  8  A2 37 26 81 A3 E9 40 40 
  1010)      4037.2  Rx         0001  8  8F B5 C9 3F 64 A9 42 40 
  1014)      4053.2  Rx         0001  8  D5 E2 3A 4E 2C 66 44 40 
  1018)      4069.2  Rx         0001  8  D7 F7 51 FB 89 1B 46 40 
  1022)      4085.2  Rx         0001  8  E2 40 B5 8D 1E C5 47 40 
  1026)      4101.2  Rx         0001  8  70 53 65 71 A9 5E 49 40 
  1030)      4117.2  Rx         0001  8  81 4C 2D 18 13 E4 4A 40 
  1034)      4133.2  Rx         0001  8  93 DE 24 71 77 51 4C 40 
  1038)      4149.2  Rx         0001  8  B9 5B 7E DC 2F A3 4D 40 
  1042)      4165.3  Rx         0001  8  75 C9 2B 82 DC D5 4E 40 
  1046)      4181.3  Rx         0001  8  6F 0A 78 F3 6C E6 4F 40 
   """

# """
#       5)         17.2 Rx          0004 8 07 BC AF D5 37 F9 DF 3F 
#       9)         33.2 Rx          0004 8 D2 FB 99 24 8B 95 E1 3F 
#      13)         49.2 Rx          0004 8 31 C2 C2 0A 6D 2A E3 3F 
#      17)         65.2 Rx          0004 8 BE 24 A0 FA 35 B7 E4 3F 
#      21)         81.2 Rx          0004 8 A3 C9 DC 07 EF 37 E6 3F 
#      25)         97.2 Rx          0004 8 10 FE 89 20 C0 A8 E7 3F 
#      29)        113.2 Rx          0004 8 B7 9A 3E E2 F9 05 E9 3F 
#      33)        129.2 Rx          0004 8 6E 62 24 07 1F 4C EA 3F 
#      37)        145.2 Rx          0004 8 60 96 D7 52 ED 77 EB 3F 
#      41)        161.2 Rx          0004 8 AF 58 44 E8 65 86 EC 3F 
#      45)        177.2 Rx          0004 8 DC B8 2F F3 D4 74 ED 3F 
#      49)        193.2 Rx          0004 8 14 11 D3 91 D8 40 EE 3F 
#      53)        209.2 Rx          0004 8 E5 48 DB EC 66 E8 EE 3F 
#      57)        225.2 Rx          0004 8 62 B9 35 6E D3 69 EF 3F 
#      61)        241.2 Rx          0004 8 A2 74 54 09 D3 C3 EF 3F 
#      65)        257.2 Rx          0004 8 C6 D5 F5 89 7F F5 EF 3F 
#      69)        273.2 Rx          0004 8 69 66 F9 E0 59 FE EF 3F 
#      73)        289.2 Rx          0004 8 3A F2 5F 69 4B DE EF 3F 
#      77)        305.2 Rx          0004 8 F8 79 37 22 A6 95 EF 3F 
#      81)        321.2 Rx          0004 8 F3 DD DE DC 23 25 EF 3F 
#      85)        337.2 Rx          0004 8 E2 B9 B9 61 E4 8D EE 3F 
#      89)        353.2 Rx          0004 8 7F 31 14 90 6A D1 ED 3F 
#      93)        369.2 Rx          0004 8 38 8D 91 80 98 F1 EC 3F 
#      97)        385.2 Rx          0004 8 E4 D5 08 B4 AA F0 EB 3F 
#     101)        401.2 Rx          0004 8 C2 A3 21 5B 32 D1 EA 3F 
#     105)        417.2 Rx          0004 8 3C D6 52 C5 0E 96 E9 3F 
#     109)        433.2 Rx          0004 8 24 FE 0E 08 66 42 E8 3F 
#     113)        449.2 Rx          0004 8 00 62 E9 F0 9C D9 E6 3F 
#     117)        465.2 Rx          0004 8 58 90 4D 57 4E 5F E5 3F 
#     121)        481.2 Rx          0004 8 04 CE FD E3 41 D7 E3 3F 
#     125)        497.2 Rx          0004 8 21 3D EE 65 62 45 E2 3F 
#     129)        513.2 Rx          0004 8 A8 D3 37 CD B3 AD E0 3F 
#     133)        529.2 Rx          0004 8 17 62 88 CB 91 28 DE 3F 
#     137)        545.2 Rx          0004 8 8A 98 EF D7 71 FA DA 3F 
#     141)        561.2 Rx          0004 8 DB 55 02 2C 2A D9 D7 3F 
#     145)        577.2 Rx          0004 8 20 BB 21 59 BC CC D4 3F 
#     149)        593.2 Rx          0004 8 81 26 2D 9B F4 DC D1 3F 
#     153)        609.2 Rx          0004 8 F8 6C D0 CC AB 22 CE 3F 
#     157)        625.2 Rx          0004 8 AC C8 A2 61 0D E2 C8 3F 
#     161)        641.2 Rx          0004 8 D0 FC 1C 50 7D 05 C4 3F 
# """

# --- Execution ---
print("--- IEEE 754 Double-Precision (64-bit) Conversion ---")
print("All hex values are treated as Little-Endian (LSB first).\n")

conversion_results = convert_hex_to_decimal(input_data)

# Print the results in a formatted table
print(f"{'Line Index':<10} | {'Hex Bytes (LE)':<30} | {'Decimal Value':<20}")
print("-" * 65)

for result in conversion_results:
    # Extract the line index from the original line for clean display
    line_index_match = re.match(r'^\s*(\d+)\)', result['original_line'])
    line_index = line_index_match.group(1) if line_index_match else 'N/A'
    
    print(f"{line_index:<10} | {result['hex_bytes_le']:<30} | {result['decimal_value']:<20.17f}")

print("-" * 65)
print(f"Total conversions: {len(conversion_results)}")
