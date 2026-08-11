#include "map_data.h"
// Simplified from Natural Earth 1:10m Admin-0 Countries and Lakes (public domain).
static const MapVertex ch[]={{5956,46132},{6077,46263},{6855,46443},{6842,46868},{7493,47066},{7594,47586},{8570,47806},{9566,47540},{9650,47050},{10492,46850},{10163,46227},{9280,46230},{8455,45820},{7860,45916},{7045,45922},{5956,46132}};
static const MapVertex fr[]={{5000,48000},{6140,47800},{7594,47586},{7493,47066},{6842,46868},{6855,46443},{6077,46263},{5956,46132},{5000,45700}};
static const MapVertex de[]={{6140,47800},{7600,48250},{9000,48300},{9566,47540},{8570,47806},{7594,47586}};
static const MapVertex at[]={{9566,47540},{11500,47800},{11500,46500},{10492,46850},{9650,47050}};
static const MapVertex it[]={{5956,46132},{7045,45922},{7860,45916},{8455,45820},{9280,46230},{10163,46227},{10492,46850},{11500,46100},{11500,45300},{5000,45300}};
static const MapVertex geneva[]={{6020,46200},{6160,46280},{6360,46320},{6480,46260},{6350,46190},{6150,46160},{6020,46200}};
static const MapVertex constance[]={{8800,47650},{9130,47750},{9510,47670},{9290,47540},{8900,47520},{8800,47650}};
static const MapVertex neuchatel[]={{6850,46800},{7120,47000},{7090,47080},{6810,46880},{6850,46800}};
static const MapVertex zurich[]={{8380,47200},{8700,47400},{8730,47280},{8440,47120},{8380,47200}};
static const MapVertex maggiore[]={{8460,45900},{8700,46180},{8790,46120},{8580,45820}};
const MapLine kMapLines[]={
 {fr,sizeof(fr)/sizeof(*fr),false,false},{de,sizeof(de)/sizeof(*de),false,false},{at,sizeof(at)/sizeof(*at),false,false},{it,sizeof(it)/sizeof(*it),false,false},{ch,sizeof(ch)/sizeof(*ch),true,false},{geneva,sizeof(geneva)/sizeof(*geneva),false,true},{constance,sizeof(constance)/sizeof(*constance),false,true},{neuchatel,sizeof(neuchatel)/sizeof(*neuchatel),false,true},{zurich,sizeof(zurich)/sizeof(*zurich),false,true},{maggiore,sizeof(maggiore)/sizeof(*maggiore),false,true}};
const uint8_t kMapLineCount=sizeof(kMapLines)/sizeof(*kMapLines);
