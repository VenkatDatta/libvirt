/*
 * lxcdocker2xmltest.c: Test LXC Docker Configuration
 *
 * Copyright (C) 2017 Venkat Datta N H
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author: Venkat Datta N H <nhvenkatdatta@gmail.com>
 */

#include <config.h>

#include "testutils.h"

#ifdef WITH_LXC

# include "lxc/lxc_docker.h"
# include "lxc/lxc_conf.h"
# include "testutilslxc.h"

# define VIR_FROM_THIS VIR_FROM_NONE

static virCapsPtr caps;
static virDomainXMLOptionPtr xmlopt;

static int testSanitizeDef(virDomainDefPtr vmdef)
{
    /* Remove UUID randomness */
    if (virUUIDParse("c7a5fdbd-edaf-9455-926a-d65c16db1809", vmdef->uuid) < 0)
        return -1;
    return 0;
}

static int
testCompareXMLToConfigFiles(const char *xmlfile,
                            const char *configfile,
                            bool expectError)
{
    int ret = -1;
    char *config = NULL;
    char *actualxml = NULL;
    virDomainDefPtr vmdef = NULL;

    if (virTestLoadFile(configfile, &config) < 0)
        goto fail;

    vmdef = virLXCDockerParseJSONConfig(caps, xmlopt, config);

    if ((vmdef && expectError) || (!vmdef && !expectError))
        goto fail;

    if (vmdef) {
        if (testSanitizeDef(vmdef) < 0)
            goto fail;

        if (!(actualxml = virDomainDefFormat(vmdef, caps, 0)))
            goto fail;

        if (virTestCompareToFile(actualxml, xmlfile) < 0)
            goto fail;
    }

    ret = 0;

 fail:
    VIR_FREE(actualxml);
    VIR_FREE(config);
    virDomainDefFree(vmdef);
    return ret;
}

struct testInfo {
    const char *name;
    bool expectError;
};

static int
testCompareXMLToConfigHelper(const void *data)
{
    int result = -1;
    const struct testInfo *info = data;
    char *xml = NULL;
    char *config = NULL;

    if (virAsprintf(&xml, "%s/lxcdocker2xmldata/lxcdocker2xmldata-%s.xml",
                    abs_srcdir, info->name) < 0 ||
        virAsprintf(&config, "%s/lxcdocker2xmldata/lxcdocker2xmldata-%s.json",
                    abs_srcdir, info->name) < 0)
        goto cleanup;

    result = testCompareXMLToConfigFiles(xml, config, info->expectError);

 cleanup:
    VIR_FREE(xml);
    VIR_FREE(config);
    return result;
}

static int
mymain(void)
{
    int ret = EXIT_SUCCESS;

    if (!(caps = testLXCCapsInit()))
        return EXIT_FAILURE;

    if (!(xmlopt = lxcDomainXMLConfInit())) {
        virObjectUnref(caps);
        return EXIT_FAILURE;
    }


# define DO_TEST(name, expectError)                         \
    do {                                                    \
        const struct testInfo info = { name, expectError }; \
        if (virTestRun("DOCKER JSON-2-XML " name,            \
                       testCompareXMLToConfigHelper,        \
                       &info) < 0)                          \
            ret = EXIT_FAILURE;                             \
    } while (0)

    DO_TEST("simple", false);

    virObjectUnref(xmlopt);
    virObjectUnref(caps);

    return ret;
}

VIR_TEST_MAIN(mymain)

#else

int
main(void)
{
    return EXIT_AM_SKIP;
}

#endif /* WITH_LXC */
