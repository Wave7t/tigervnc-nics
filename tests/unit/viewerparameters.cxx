/* Copyright 2026 TigerVNC team
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <string>

#include <gtest/gtest.h>

#include "parameters.h"

class ViewerParametersTest : public testing::Test {
protected:
  void SetUp() override
  {
    filename = testing::TempDir() + "tigervnc-ssh-profile.tigervnc";
    via.setParam("");
  }

  void TearDown() override
  {
    via.setParam("");
    remove(filename.c_str());
  }

  std::string filename;
};

TEST_F(ViewerParametersTest, savesAndLoadsSshGateway)
{
  via.setParam("viewer@vnc-gateway");

  saveViewerParameters(filename.c_str(), "desktop.internal:2");

  via.setParam("");
  const char* serverName = loadViewerParameters(filename.c_str());

  ASSERT_NE(serverName, nullptr);
  EXPECT_STREQ(serverName, "desktop.internal:2");
  EXPECT_STREQ(via, "viewer@vnc-gateway");
}
