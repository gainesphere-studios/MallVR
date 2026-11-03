/* The other Virtuality 1000-series rebuilds, on the kit in vpix_dactyl1k.h. Switched on by VPIX_NATIVE=<name>. Dev only, not in a release. */
#include "vpix_1k_exorex.h"
#include "vpix_1k_aces.h"
#include "vpix_1k_legend.h"
#include "vpix_1k_gridwar.h"
#include "vpix_1k_gridbust.h"
#include "vpix_1k_vtol.h"
#include "vpix_1k_totald.h"
#include "vpix_1k_sphere.h"
#include "vpix_1k_hero.h"
#include "vpix_2k_sphere.h"
#include "vpix_2k_xtreme.h"
namespace v1k {
static const struct { const char *name; void (*start)(const char *); } games[] = {
    {"exorex", exorex::start}, {"aces", aces::start}, {"legend", legend::start}, {"gridwar", gridwar::start}, {"gridbust", gridbust::start},
    {"vtol", vtol::start}, {"totald", totald::start}, {"sphere", sphere::start}, {"sphere2k", sphere2k::start}, {"xtreme", xtreme::start}, {"hero", hero::start},
};
static void (*find(const char *name))(const char *) {
    if (name) for (const auto &g : games) if (!strcmp(g.name, name)) return g.start;
    return NULL;
}
}   /* namespace v1k */
