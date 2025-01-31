/*
 * ip4.h -- fallback parser for IPv4 addresses
 *
 * Copyright (c) 2022-2023, NLnet Labs. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef IP4_H
#define IP4_H

nonnull_all
static really_inline int32_t scan_ip4(
  const char *text, uint8_t *wire, size_t *length)
{
  const char *start = text;
  uint32_t round = 0;
  for (;;) {
    uint8_t digits[3];
    uint32_t octet;
    digits[0] = (uint8_t)text[0] - '0';
    digits[1] = (uint8_t)text[1] - '0';
    digits[2] = (uint8_t)text[2] - '0';
    if (digits[0] > 9)
      return -1;
    else if (digits[1] > 9)
      (void)(text += 1), octet = digits[0];
    else if (digits[2] > 9)
      (void)(text += 2), octet = digits[0] * 10 + digits[1];
    else
      (void)(text += 3), octet = digits[0] * 100 + digits[1] * 10 + digits[2];

    if (octet > 255)
      return -1;
    wire[round++] = (uint8_t)octet;
    if (text[0] != '.' || round == 4)
      break;
    text += 1;
  }

  if (round != 4)
    return -1;
  *length = (uintptr_t)text - (uintptr_t)start;
  return 4;
}

nonnull_all
static really_inline int32_t parse_ip4(
  parser_t *parser,
  const type_info_t *type,
  const rdata_info_t *item,
  rdata_t *rdata,
  const token_t *token)
{
  uint8_t *o = rdata->octets;
  const uint8_t *os = o;
  uint64_t n = 0;
  const char *p = token->data;
  static const uint8_t m[] = { 0, 0, 10, 100 };

  *o = 0;
  for (const char *ps = p;; p++) {
    const uint64_t d = (uint8_t)*p - '0';
    if (d <= 9) {
      n = n * 10 + (uint8_t)d;
    } else {
      if (!(p - ps) || p - ps > 3 || n < m[(p - ps)] || n > 255 || o - os > 3)
        SYNTAX_ERROR(parser, "Invalid %s in %s", NAME(item), NAME(type));
      ps = p + 1;
      *o++ = (uint8_t)n;
      if (*p != '.')
        break;
      n = 0;
    }
  }

  if (p != token->data + token->length || o - os != 4)
    SYNTAX_ERROR(parser, "Invalid %s in %s", NAME(item), NAME(type));

  rdata->octets += 4;
  return 0;
}

#endif // IP4_H
