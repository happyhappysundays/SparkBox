// ========================================================================================
// ========================================================================================
// ========================================================================================
// ============== Taken from here https://github.com/paulhamsh/SparkIO ====================
// ============================= Slightly modified ========================================
// ========================================================================================
// ========================================================================================
// ========================================================================================

#include <Arduino.h>
#include "Spark.h"

typedef struct {
  int fxSlot;
  int fxNumber;
} s_fx_coords;

const SparkPreset preset0{0x0,0x7f,"07079063-94A9-41B1-AB1D-02CBC5D00790","Silver Ship","0.7","1-Clean","icon.png",120.000000,{ 
  {"bias.noisegate", false, 3, {0.138313, 0.224643, 0.000000}}, 
  {"LA2AComp", true, 3, {0.000000, 0.852394, 0.373072}}, 
  {"Booster", false, 1, {0.722592}}, 
  {"RolandJC120", true, 5, {0.632231, 0.281820, 0.158359, 0.671320, 0.805785}}, 
  {"Cloner", true, 2, {0.199593, 0.000000}}, 
  {"VintageDelay", false, 4, {0.378739, 0.425745, 0.419816, 1.000000}}, 
  {"bias.reverb", true, 7, {0.285714, 0.408354, 0.289489, 0.388317, 0.582143, 0.650000, 0.200000}} },0xb4 };

const SparkPreset preset1{0x0,0x7f,"CDE99591-C05D-4AE0-9E34-EC4A81F3F84F","Sweet Memory","0.7","1-Clean","icon.png",120.000000,{ 
  {"bias.noisegate", false, 3, {0.099251, 0.570997, 0.000000}}, 
  {"BlueComp", false, 4, {0.430518, 0.663291, 0.355048, 0.557014}}, 
  {"DistortionTS9", false, 3, {0.058011, 0.741722, 0.595924}}, 
  {"94MatchDCV2", true, 5, {0.528926, 0.500905, 0.246163, 0.417119, 0.782293}}, 
  {"Flanger", false, 3, {0.413793, 0.663043, 0.655172}}, 
  {"DelayRe201", true, 5, {0.097778, 0.312182, 0.485182, 0.369640, 1.000000}}, 
  {"bias.reverb", true, 7, {0.561185, 0.506659, 0.417857, 0.300847, 0.602287, 0.594118, 0.000000}} },0xeb };

const SparkPreset preset2{0x0,0x7f,"F577F7F3-E8E0-4D35-8975-0427C2054DCE","Dancing in the room","0.7","Description for Blues Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", false, 2, {0.283019, 0.304245}}, 
  {"Compressor", true, 2, {0.325460, 0.789062}}, 
  {"Booster", false, 1, {0.666735}}, 
  {"Twin", true, 5, {0.613433, 0.371715, 0.453167, 0.676660, 0.805785}}, 
  {"ChorusAnalog", true, 4, {0.185431, 0.086409, 0.485027, 0.567797}}, 
  {"DelayEchoFilt", false, 5, {0.533909, 0.275554, 0.455372, 0.457702, 1.000000}}, 
  {"bias.reverb", true, 7, {0.508871, 0.317935, 0.461957, 0.349689, 0.339286, 0.481753, 0.700000}} },0x48 };

const SparkPreset preset3{0x0,0x7f,"D8757D67-98EA-4888-86E5-5F1FD96A30C3","Royal Crown","0.7","1-Clean","icon.png",120.000000,{ 
  {"bias.noisegate", true, 3, {0.211230, 0.570997, 0.000000}}, 
  {"Compressor", true, 2, {0.172004, 0.538197}}, 
  {"DistortionTS9", false, 3, {0.703110, 0.278146, 0.689846}}, 
  {"ADClean", true, 5, {0.677083, 0.501099, 0.382828, 0.585946, 0.812231}}, 
  {"ChorusAnalog", true, 4, {0.519976, 0.402152, 0.240642, 0.740579}}, 
  {"DelayMono", true, 5, {0.173729, 0.233051, 0.493579, 0.600000, 1.000000}}, 
  {"bias.reverb", true, 7, {0.688801, 0.392857, 0.461138, 0.693705, 0.488235, 0.466387, 0.300000}} },0xa2 };

const SparkPreset preset4{0x0,0x7f,"9D2F2AA3-4EC5-4BD7-A3CD-A76FD55698DB","Wooden Bridge","0.7","Description for Blues Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.316873, 0.304245}}, 
  {"Compressor", false, 2, {0.341085, 0.665754}}, 
  {"Booster", true, 1, {0.661412}}, 
  {"Bassman", true, 5, {0.768152, 0.491509, 0.476547, 0.284314, 0.389779}}, 
  {"UniVibe", false, 3, {0.500000, 1.000000, 0.700000}}, 
  {"VintageDelay", true, 4, {0.152219, 0.663314, 0.144982, 1.000000}}, 
  {"bias.reverb", true, 7, {0.120109, 0.150000, 0.500000, 0.406755, 0.299253, 0.768478, 0.100000}} },0x12 };

const SparkPreset preset5{0x0,0x7f,"B08F2421-0686-484E-B6EC-8F660A9344FC","Stone Breaker","0.7","Description for Blues Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.105936, 0.231329}}, 
  {"Compressor", true, 2, {0.341085, 0.665754}}, 
  {"DistortionTS9", false, 3, {0.117948, 0.390437, 0.583560}}, 
  {"TwoStoneSP50", true, 5, {0.634593, 0.507692, 0.664699, 0.519608, 0.714050}}, 
  {"Tremolator", false, 3, {0.330000, 0.500000, 1.000000}}, 
  {"DelayRe201", true, 5, {0.324783, 0.204820, 0.460643, 0.304200, 1.000000}}, 
  {"bias.reverb", true, 7, {0.554974, 0.842373, 0.783898, 0.385087, 0.659664, 0.294118, 0.000000}} },0x59 };

const SparkPreset preset6{0x0,0x7f,"55D60EB5-1735-4746-B0C4-16C53D8CA203","Country road","0.7","Description for Blues Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.283019, 0.304245}}, 
  {"Compressor", false, 2, {0.461066, 0.608902}}, 
  {"DistortionTS9", true, 3, {0.200747, 0.216084, 0.583560}}, 
  {"AC Boost", true, 5, {0.707792, 0.591124, 0.383605, 0.532821, 0.195119}}, 
  {"Tremolo", false, 3, {0.454134, 0.699934, 0.596154}}, 
  {"DelayRe201", false, 5, {0.331450, 0.348991, 0.672299, 0.453144, 1.000000}}, 
  {"bias.reverb", true, 7, {0.622826, 0.150000, 0.500000, 0.621429, 0.369905, 0.350000, 0.100000}} },0x57 };

const SparkPreset preset7{0x0,0x7f,"2E2928B5-D87E-4346-B58F-145B88C581BE","Blues Ark","0.7","1-Clean","icon.png",120.000000,{ 
  {"bias.noisegate", true, 3, {0.127897, 0.313185, 0.000000}}, 
  {"LA2AComp", true, 3, {0.000000, 0.832474, 0.304124}}, 
  {"DistortionTS9", true, 3, {0.570513, 0.549669, 0.706421}}, 
  {"Twin", true, 5, {0.679549, 0.371715, 0.593663, 0.676660, 0.479191}}, 
  {"ChorusAnalog", false, 4, {0.377119, 0.310128, 0.510580, 0.455357}}, 
  {"DelayMono", true, 5, {0.173729, 0.239186, 0.521186, 0.606780, 1.000000}}, 
  {"bias.reverb", true, 7, {0.325512, 0.392857, 0.461138, 0.100520, 0.488235, 0.466387, 0.300000}} },0xa3 };

const SparkPreset preset8{0x0,0x7f,"BB40E550-77D0-40B1-B0D3-D15D3D0C19EE","Modern Stone","0.7","Description for Rock Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.271226, 0.370283}}, 
  {"BlueComp", true, 4, {0.389830, 0.665254, 0.305085, 0.644068}}, 
  {"Overdrive", false, 3, {0.586207, 0.500288, 0.530172}}, 
  {"AmericanHighGain", true, 5, {0.616274, 0.431090, 0.419846, 0.495112, 0.850637}}, 
  {"ChorusAnalog", false, 4, {0.120593, 0.279661, 0.185763, 0.485297}}, 
  {"DelayMono", false, 5, {0.271017, 0.190613, 0.355976, 0.555932, 0.000000}}, 
  {"bias.reverb", true, 7, {0.175725, 0.680389, 0.761837, 0.177584, 0.302521, 0.408933, 0.300000}} },0xe6 };

const SparkPreset preset9{0x0,0x7f,"BFCFC107-6E80-4F26-8549-F44638856241","Crazy Crue","0.7","1-Clean","icon.png",120.000000,{ 
  {"bias.noisegate", true, 3, {0.104459, 0.232455, 0.000000}}, 
  {"Compressor", false, 2, {0.683015, 0.327260}}, 
  {"DistortionTS9", true, 3, {0.355043, 0.314570, 0.554487}}, 
  {"YJM100", true, 5, {0.562574, 0.485294, 0.317001, 0.250528, 0.576942}}, 
  {"ChorusAnalog", false, 4, {0.127396, 0.172299, 0.745763, 0.750884}}, 
  {"DelayRe201", true, 5, {0.071111, 0.223224, 0.285796, 0.537533, 1.000000}}, 
  {"bias.reverb", true, 7, {0.147366, 0.506659, 0.417857, 0.268239, 0.602287, 0.594118, 0.000000}} },0xcc };

const SparkPreset preset10{0x0,0x7f,"6AF9D829-CEA7-4189-AC80-B3364A563EB4","Dark Soul","0.7","1-Clean","icon.png",120.000000,{ 
  {"bias.noisegate", true, 3, {0.116817, 0.128289, 0.000000}}, 
  {"BBEOpticalComp", false, 3, {0.712698, 0.370691, 0.000000}}, 
  {"Overdrive", true, 3, {0.586207, 0.334725, 0.256692}}, 
  {"SLO100", true, 5, {0.590909, 0.512066, 0.583825, 0.287179, 0.507674}}, 
  {"Flanger", false, 3, {0.413793, 0.663043, 0.655172}}, 
  {"DelayMono", false, 5, {0.215111, 0.192443, 0.478663, 0.400000, 1.000000}}, 
  {"bias.reverb", true, 7, {0.340062, 0.809783, 0.295483, 0.149187, 0.582143, 0.650000, 0.200000}} },0x58 };

const SparkPreset preset11{0x0,0x7f,"7984DF4F-5885-4FE2-9786-F3F31E322E44","British Accent","0.7","Description for Pop Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.095322, 0.242286}}, 
  {"Compressor", false, 2, {0.499939, 0.337629}}, 
  {"Booster", true, 1, {0.602726}}, 
  {"OrangeAD30", true, 5, {0.620474, 0.312894, 0.484227, 0.527442, 0.492836}}, 
  {"Cloner", false, 2, {0.500000, 0.000000}}, 
  {"DelayRe201", false, 5, {0.224783, 0.324451, 0.153894, 0.488644, 1.000000}}, 
  {"bias.reverb", true, 7, {0.205106, 0.721662, 0.656790, 0.193705, 0.488235, 0.466387, 0.300000}} },0xca };

const SparkPreset preset12{0x0,0x7f,"96E26248-0AA3-4D45-B767-8BB9337346C9","Iron Hammer","0.7","Description for Metal Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.217472, 0.000000}}, 
  {"Compressor", true, 2, {0.547004, 0.647572}}, 
  {"DistortionTS9", false, 3, {0.147861, 0.400662, 0.640123}}, 
  {"SwitchAxeLead", true, 5, {0.572609, 0.352941, 0.374004, 0.460784, 0.705431}}, 
  {"UniVibe", false, 3, {0.500000, 1.000000, 0.700000}}, 
  {"DelayRe201", false, 5, {0.207006, 0.281507, 0.411563, 0.461977, 1.000000}}, 
  {"bias.reverb", false, 7, {0.103649, 0.720854, 0.531337, 0.189948, 0.631056, 0.380978, 0.200000}} },0x49 };

const SparkPreset preset13{0x0,0x7f,"82C5B8E8-7889-4302-AC19-74DF543872E1","Millenial Lead","0.7","Description for Metal Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.209660, 0.064809}}, 
  {"BlueComp", false, 4, {0.392063, 0.665254, 0.452324, 0.597193}}, 
  {"DistortionTS9", true, 3, {0.046116, 0.357616, 0.769957}}, 
  {"BE101", true, 5, {0.587958, 0.343137, 0.475797, 0.394193, 0.875443}}, 
  {"ChorusAnalog", false, 4, {0.761324, 0.132422, 0.491161, 0.567797}}, 
  {"DelayMono", true, 5, {0.161777, 0.180514, 0.500135, 0.600000, 1.000000}}, 
  {"bias.reverb", true, 7, {0.095109, 0.660326, 0.692935, 0.285326, 0.500000, 0.500000, 0.300000}} },0x1 };

const SparkPreset preset14{0x0,0x7f,"5F27120E-5119-4923-ADA9-42CCB5B01A95","Heavy Axe","0.7","Description for Metal Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.108097, 0.051788}}, 
  {"BBEOpticalComp", false, 3, {0.712698, 0.441540, 0.000000}}, 
  {"DistortionTS9", true, 3, {0.080701, 0.298013, 0.554487}}, 
  {"EVH", true, 5, {0.599518, 0.467647, 0.407468, 0.357744, 0.820512}}, 
  {"Phaser", false, 2, {0.331250, 0.620000}}, 
  {"DelayMono", false, 5, {0.228444, 0.180514, 0.463325, 0.400000, 1.000000}}, 
  {"bias.reverb", false, 7, {0.285714, 0.709984, 0.582967, 0.388317, 0.582143, 0.650000, 0.200000}} },0xe4 };

const SparkPreset preset15{0x0,0x7f,"961F7F40-77C3-4E98-A694-DF9CA4069955","Dual Train","0.7","Description for Rock Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.148831, 0.000000}}, 
  {"BBEOpticalComp", true, 3, {0.707544, 0.526591, 0.000000}}, 
  {"DistortionTS9", true, 3, {0.016884, 0.370637, 0.719230}}, 
  {"Rectifier", true, 5, {0.706630, 0.425070, 0.450462, 0.498249, 0.795350}}, 
  {"Cloner", false, 2, {0.330986, 0.000000}}, 
  {"DelayMono", false, 5, {0.226572, 0.140636, 0.550847, 0.555932, 0.000000}}, 
  {"bias.reverb", false, 7, {0.366912, 0.672237, 0.314634, 0.284543, 0.302521, 0.433390, 0.300000}} },0xc6 };

const SparkPreset preset16{0x0,0x7f,"94109418-E7D9-4B99-83F7-DDB11CA5847D","Spooky Melody","0.7","Description for Alternative Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.500000, 1.000000}}, 
  {"Compressor", true, 2, {0.351691, 0.354167}}, 
  {"DistortionTS9", false, 3, {0.272170, 0.642384, 0.595924}}, 
  {"Twin", true, 5, {0.613433, 0.489362, 0.453167, 0.505091, 0.580000}}, 
  {"UniVibe", true, 3, {0.636598, 0.000000, 0.493814}}, 
  {"DelayEchoFilt", true, 5, {0.231858, 0.555041, 0.529055, 0.308814, 0.000000}}, 
  {"bias.reverb", true, 7, {0.963044, 0.232082, 0.176398, 0.224767, 0.228167, 0.357143, 0.500000}} },0x19 };

const SparkPreset preset17{0x0,0x7f,"E237C4CF-172B-4D68-AA5B-659F57715658","Fuzzy Jam","0.7","Description for Alternative Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.500000, 1.000000}}, 
  {"Compressor", true, 2, {0.435025, 0.647572}}, 
  {"Fuzz", true, 2, {0.436505, 1.000000}}, 
  {"ADClean", true, 5, {0.677083, 0.364470, 0.353902, 0.341186, 0.680000}}, 
  {"UniVibe", false, 3, {0.500000, 1.000000, 0.700000}}, 
  {"VintageDelay", false, 4, {0.293103, 0.646739, 0.284055, 1.000000}}, 
  {"bias.reverb", true, 7, {0.493323, 0.293282, 0.520823, 0.398143, 0.469538, 0.455462, 0.600000}} },0x13 };

const SparkPreset preset18{0x0,0x7f,"A3601E1D-8018-42A8-9A19-9B6F0DAB6F46","Angry Monkey","0.7","Description for Alternative Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.500000, 1.000000}}, 
  {"BlueComp", true, 4, {0.389830, 0.665254, 0.305085, 0.644068}}, 
  {"GuitarMuff", true, 3, {0.619421, 0.692053, 0.805691}}, 
  {"94MatchDCV2", true, 5, {0.512397, 0.557982, 0.415584, 0.438462, 0.351240}}, 
  {"Flanger", true, 3, {1.000000, 0.338540, 0.245856}}, 
  {"DelayMono", true, 5, {0.096667, 0.101227, 0.395705, 0.320000, 1.000000}}, 
  {"bias.reverb", true, 7, {0.554341, 0.308929, 0.237733, 0.738432, 0.265140, 0.276786, 0.400000}} },0xe3 };

const SparkPreset preset19{0x0,0x7f,"50A3B945-1A86-4E06-B10B-550E3226DDF2","Hide and Seek","0.7","Description for Alternative Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.500000, 1.000000}}, 
  {"BlueComp", false, 4, {0.590723, 0.665254, 0.305085, 0.644068}}, 
  {"Booster", true, 1, {0.668454}}, 
  {"Bogner", true, 5, {0.655844, 0.626593, 0.640734, 0.351588, 0.338571}}, 
  {"MiniVibe", false, 2, {0.047057, 0.117188}}, 
  {"DelayMultiHead", true, 5, {0.706667, 0.644172, 0.564417, 0.650000, 1.000000}}, 
  {"bias.reverb", true, 7, {0.448518, 0.405932, 0.566185, 0.648674, 0.302521, 0.180672, 0.300000}} },0x45 };

const SparkPreset preset20{0x0,0x7f,"3013444B-9929-499F-964D-707E9D8F5FA0","Bass Driver","0.7","Description for Bass Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.419271, 0.226562}}, 
  {"BassComp", true, 2, {0.372727, 0.530303}}, 
  {"SABdriver", true, 4, {0.535256, 1.000000, 0.724359, 1.000000}}, 
  {"W600", true, 5, {0.664699, 0.423077, 0.276884, 0.415083, 0.448052}}, 
  {"UniVibe", false, 3, {0.500000, 1.000000, 0.700000}}, 
  {"VintageDelay", false, 4, {0.359402, 0.400883, 0.350280, 1.000000}}, 
  {"bias.reverb", true, 7, {0.253261, 0.462500, 0.234401, 0.137733, 0.293818, 0.638043, 0.100000}} },0x91 };

const SparkPreset preset21{0x0,0x7f,"D99DC07A-C997-4ABD-833A-0C13EA8BEE5A","Comped Cleaner","0.7","Description for Bass Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.205729, 0.226562}}, 
  {"BassComp", true, 2, {0.193040, 0.334991}}, 
  {"MaestroBassmaster", false, 3, {0.698052, 0.276184, 0.566086}}, 
  {"GK800", true, 5, {0.688351, 0.407152, 0.399197, 0.746875, 0.774234}}, 
  {"Cloner", false, 2, {0.248888, 0.000000}}, 
  {"DelayMono", false, 5, {0.163333, 0.214724, 0.355828, 0.320000, 1.000000}}, 
  {"bias.reverb", true, 7, {0.168478, 0.744565, 0.130435, 0.288043, 0.323370, 0.293478, 0.600000}} },0x54 };

const SparkPreset preset22{0x0,0x7f,"57AF2690-F7C8-4766-9A92-F3A51629B959","Cozy Serenade","0.7","Description for Acoustic Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.143229, 0.000000}}, 
  {"BlueComp", true, 4, {0.506411, 0.356543, 0.348913, 0.407461}}, 
  {"Booster", false, 1, {0.567515}}, 
  {"FatAcousticV2", true, 5, {0.453955, 0.292760, 0.565172, 0.575339, 0.829431}}, 
  {"ChorusAnalog", false, 4, {0.761324, 0.236716, 0.745763, 0.431636}}, 
  {"DelayMono", false, 5, {0.187778, 0.211656, 0.334356, 0.400000, 1.000000}}, 
  {"bias.reverb", true, 7, {0.315883, 0.652174, 0.111413, 0.357842, 0.339286, 0.489905, 0.700000}} },0x2f };

const SparkPreset preset23{0x0,0x7f,"DEFBB271-B3EE-4C7E-A623-2E5CA53B6DDA","Studio Session","0.7","Description for Acoustic Preset 1","icon.png",120.000000,{ 
  {"bias.noisegate", false, 2, {0.500000, 0.346698}}, 
  {"BBEOpticalComp", true, 3, {0.758266, 0.258550, 0.000000}}, 
  {"DistortionTS9", false, 3, {0.139574, 0.407285, 0.689846}}, 
  {"Acoustic", true, 5, {0.639823, 0.385056, 0.383449, 0.599397, 0.519480}}, 
  {"ChorusAnalog", true, 4, {0.841681, 0.227514, 0.935947, 0.351279}}, 
  {"DelayMono", false, 5, {0.223999, 0.211189, 0.490933, 0.600000, 1.000000}}, 
  {"bias.reverb", true, 7, {0.722819, 0.326169, 0.275776, 0.360714, 0.343944, 0.486025, 0.400000}} },0x23 };

const SparkPreset *my_presets[]{&preset0,  &preset1,  &preset2,  &preset3,  &preset4,  &preset5,  &preset6,  &preset7,  &preset8,  &preset9,  
              &preset10, &preset11, &preset12, &preset13, &preset14, &preset15, &preset16, &preset17, &preset18, &preset19, 
              &preset20, &preset21, &preset22, &preset23};

const char spark_noisegates[][STR_LEN+1]{"bias.noisegate"};
const char spark_compressors[][STR_LEN+1]{"BassComp","BBEOpticalComp","BlueComp","Compressor","JH.Vox846","LA2AComp"};
const char spark_drives[][STR_LEN+1]{"BassBigMuff","Booster","DistortionTS9","Fuzz","GuitarMuff","KlonCentaurSilver","MaestroBassmaster",
  "Overdrive","ProCoRat","SABdriver","TrebleBooster"};
const char spark_amps[][STR_LEN+1]{"6505Plus","94MatchDCV2","AC Boost","Acoustic","AcousticAmpV2","ADClean","AmericanHighGain","Bassman",
  "BE101","BluesJrTweed","Bogner","Checkmate","Deluxe65","EVH","FatAcousticV2","FlatAcoustic","GK800","Hammer500","Invader","JH.JTM45",
  "JH.SuperLead100","ODS50CN","OrangeAD30","OverDrivenJM45","OverDrivenLuxVerb","Plexi","Rectifier","RolandJC120","SLO100","Sunny3000",
  "SwitchAxeLead","Twin","TwoStoneSP50","W600","YJM100"};
const char spark_modulations[][STR_LEN+1]{"ChorusAnalog","Cloner","Flanger","GuitarEQ6","JH.VoodooVibeJr","MiniVibe","Phaser",
  "Tremolator","Tremolo","TremoloSquare","UniVibe","Vibrato01"};
const char spark_delays[][STR_LEN+1]{"DelayEchoFilt","DelayMono","DelayMultiHead","DelayRe201","DelayReverse","VintageDelay"};
const char spark_reverbs[][STR_LEN+1]{"bias.reverb"};

// knob, fx, param
const char spark_knobs[7][5][12] {
  {"0-0","0-1","0-2","0-3","0-4"}, //noise gate
  {"1-0","1-1","1-2","1-3","1-4"}, //compressor
  {"DRIVE","2-1","2-2","2-3","2-4"}, //drive
  {"GAIN","TREBLE","MID","BASS","MASTER"}, //amp
  {"MODULATION","M-INTENSITY","4-2","4-3","4-4"}, //modulation
  {"DELAY","5-1","5-2","5-3","BPM"}, //delay
  {"REVERB","6-1","6-2","6-3","6-4"}  //reverb
};    

const s_fx_coords knobs_order[] = {
  {3,0},
  {3,3},
  {3,2},
  {3,1},
  {3,4},
  {4,0},
  {5,0},
  {6,0},
  {2,0}
};

const uint8_t knobs_number = 9;