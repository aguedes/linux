// SPDX-License-Identifier: GPL-2.0
/* Copyright (c)  2018 Intel Corporation */

#include "igc.h"
#include "igc_diag.h"

struct igc_reg_test reg_test[] = {
	{ IGC_FCAL,	1,	PATTERN_TEST,	0xFFFFFFFF,	0xFFFFFFFF },
	{ IGC_FCAH,	1,	PATTERN_TEST,	0x0000FFFF,	0xFFFFFFFF },
	{ IGC_FCT,	1,	PATTERN_TEST,	0x0000FFFF,	0xFFFFFFFF },
	{ IGC_RDBAH(0), 4,	PATTERN_TEST,	0xFFFFFFFF,	0xFFFFFFFF },
	{ IGC_RDBAL(0),	4,	PATTERN_TEST,	0xFFFFFF80,	0xFFFFFF80 },
	{ IGC_RDLEN(0),	4,	PATTERN_TEST,	0x000FFF80,	0x000FFFFF },
	{ IGC_RDT(0),	4,	PATTERN_TEST,	0x0000FFFF,	0x0000FFFF },
	{ IGC_FCRTH,	1,	PATTERN_TEST,	0x0003FFF0,	0x0003FFF0 },
	{ IGC_FCTTV,	1,	PATTERN_TEST,	0x0000FFFF,	0x0000FFFF },
	{ IGC_TIPG,	1,	PATTERN_TEST,	0x3FFFFFFF,	0x3FFFFFFF },
	{ IGC_TDBAH(0),	4,	PATTERN_TEST,	0xFFFFFFFF,     0xFFFFFFFF },
	{ IGC_TDBAL(0),	4,	PATTERN_TEST,	0xFFFFFF80,     0xFFFFFF80 },
	{ IGC_TDLEN(0),	4,	PATTERN_TEST,	0x000FFF80,     0x000FFFFF },
	{ IGC_TDT(0),	4,	PATTERN_TEST,	0x0000FFFF,     0x0000FFFF },
	{ IGC_RCTL,	1,	SET_READ_TEST,	0xFFFFFFFF,	0x00000000 },
	{ IGC_RCTL,	1,	SET_READ_TEST,	0x04CFB2FE,	0x003FFFFB },
	{ IGC_RCTL,	1,	SET_READ_TEST,	0x04CFB2FE,	0xFFFFFFFF },
	{ IGC_TCTL,	1,	SET_READ_TEST,	0xFFFFFFFF,	0x00000000 },
	{ IGC_RA,	16,	TABLE64_TEST_LO,
						0xFFFFFFFF,	0xFFFFFFFF },
	{ IGC_RA,	16,	TABLE64_TEST_HI,
						0x900FFFFF,	0xFFFFFFFF },
	{ IGC_MTA,	128,	TABLE32_TEST,
						0xFFFFFFFF,	0xFFFFFFFF },
	{ 0, 0, 0, 0}
};

static bool reg_pattern_test(struct igc_adapter *adapter, u64 *data, int reg,
			     u32 mask, u32 write)
{
	struct igc_hw *hw = &adapter->hw;
	u32 pat, val, before;
	static const u32 test_pattern[] = {
		0x5A5A5A5A, 0xA5A5A5A5, 0x00000000, 0xFFFFFFFF
	};

	for (pat = 0; pat < ARRAY_SIZE(test_pattern); pat++) {
		before = rd32(reg);
		wr32(reg, test_pattern[pat] & write);
		val = rd32(reg);
		if (val != (test_pattern[pat] & write & mask)) {
			dev_err(&adapter->pdev->dev,
				"pattern test reg %04X failed: got 0x%08X expected 0x%08X\n",
				reg, val, test_pattern[pat] & write & mask);
			*data = reg;
			wr32(reg, before);
			return true;
		}
		wr32(reg, before);
	}
	return false;
}

static bool reg_set_and_check(struct igc_adapter *adapter, u64 *data, int reg,
			      u32 mask, u32 write)
{
	struct igc_hw *hw = &adapter->hw;
	u32 val, before;

	before = rd32(reg);
	wr32(reg, write & mask);
	val = rd32(reg);
	if ((write & mask) != (val & mask)) {
		dev_err(&adapter->pdev->dev,
			"set/check reg %04X test failed: got 0x%08X expected 0x%08X\n",
			reg, (val & mask), (write & mask));
		*data = reg;
		wr32(reg, before);
		return true;
	}
	wr32(reg, before);
	return false;
}

bool igc_reg_test(struct igc_adapter *adapter, u64 *data)
{
	struct igc_reg_test *test = reg_test;
	struct igc_hw *hw = &adapter->hw;
	u32 value, before, after;
	u32 i, toggle, b;

	/* Because the status register is such a special case,
	 * we handle it separately from the rest of the register
	 * tests.  Some bits are read-only, some toggle, and some
	 * are writeable.
	 */
	toggle = 0x6800D3;
	before = rd32(IGC_STATUS);
	value = before & toggle;
	wr32(IGC_STATUS, toggle);
	after = rd32(IGC_STATUS) & toggle;
	if (value != after) {
		dev_err(&adapter->pdev->dev,
			"failed STATUS register test got: 0x%08X expected: 0x%08X\n",
			after, value);
		*data = 1;
		return true;
	}
	/* restore previous status */
	wr32(IGC_STATUS, before);

	/* Perform the remainder of the register test, looping through
	 * the test table until we either fail or reach the null entry.
	 */
	while (test->reg) {
		for (i = 0; i < test->array_len; i++) {
			switch (test->test_type) {
			case PATTERN_TEST:
				b = reg_pattern_test(adapter, data,
						     test->reg + (i * 0x40),
						     test->mask,
						     test->write);
				break;
			case SET_READ_TEST:
				b = reg_set_and_check(adapter, data,
						      test->reg + (i * 0x40),
						      test->mask,
						      test->write);
				break;
			case TABLE64_TEST_LO:
				b = reg_pattern_test(adapter, data,
						     test->reg + (i * 8),
						     test->mask,
						     test->write);
				break;
			case TABLE64_TEST_HI:
				b = reg_pattern_test(adapter, data,
						     test->reg + 4 + (i * 8),
						     test->mask,
						     test->write);
				break;
			case TABLE32_TEST:
				b = reg_pattern_test(adapter, data,
						     test->reg + (i * 4),
						     test->mask,
						     test->write);
				break;
			}
			if (b)
				return true;
		}
		test++;
	}
	*data = 0;
	return false;
}

bool igc_eeprom_test(struct igc_adapter *adapter, u64 *data)
{
	struct igc_hw *hw = &adapter->hw;

	*data = 0;

	if (hw->nvm.ops.validate(hw) != IGC_SUCCESS) {
		*data = 1;
		return true;
	}

	return false;
}

static irqreturn_t igc_test_intr(int irq, void *data)
{
	struct igc_adapter *adapter = (struct igc_adapter *)data;
	struct igc_hw *hw = &adapter->hw;

	adapter->test_icr |= rd32(IGC_ICR);

	return IRQ_HANDLED;
}

int igc_intr_test(struct igc_adapter *adapter, u64 *data)
{
	struct igc_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	u32 mask, ics_mask, i = 0, shared_int = true;
	u32 irq = adapter->pdev->irq;

	*data = 0;

	/* Hook up test interrupt handler just for this test */
	if (adapter->msix_entries) {
		if (request_irq(adapter->msix_entries[0].vector,
				&igc_test_intr, 0, netdev->name, adapter)) {
			*data = 1;
			return -1;
		}
	} else if (adapter->flags & IGC_FLAG_HAS_MSI) {
		shared_int = false;
		if (request_irq(irq,
				igc_test_intr, 0, netdev->name, adapter)) {
			*data = 1;
			return -1;
		}
	} else if (!request_irq(irq, igc_test_intr, IRQF_PROBE_SHARED,
				netdev->name, adapter)) {
		shared_int = false;
	} else if (request_irq(irq, &igc_test_intr, IRQF_SHARED,
		 netdev->name, adapter)) {
		*data = 1;
		return -1;
	}
	dev_info(&adapter->pdev->dev, "testing %s interrupt\n",
		 (shared_int ? "shared" : "unshared"));

	/* Disable all the interrupts */
	wr32(IGC_IMC, ~0);
	wrfl();
	usleep_range(10000, 20000);

	/* Define all writable bits for ICS */
	ics_mask = 0x77DCFED5;

	/* Test each interrupt */
	for (; i < 31; i++) {
		/* Interrupt to test */
		mask = BIT(i);

		if (!(mask & ics_mask))
			continue;

		if (!shared_int) {
			/* Disable the interrupt to be reported in
			 * the cause register and then force the same
			 * interrupt and see if one gets posted.  If
			 * an interrupt was posted to the bus, the
			 * test failed.
			 */
			adapter->test_icr = 0;

			/* Flush any pending interrupts */
			wr32(IGC_ICR, ~0);

			wr32(IGC_IMC, mask);
			wr32(IGC_ICS, mask);
			wrfl();
			usleep_range(10000, 20000);

			if (adapter->test_icr & mask) {
				*data = 3;
				break;
			}
		}

		/* Enable the interrupt to be reported in
		 * the cause register and then force the same
		 * interrupt and see if one gets posted.  If
		 * an interrupt was not posted to the bus, the
		 * test failed.
		 */
		adapter->test_icr = 0;

		/* Flush any pending interrupts */
		wr32(IGC_ICR, ~0);

		wr32(IGC_IMS, mask);
		wr32(IGC_ICS, mask);
		wrfl();
		usleep_range(10000, 20000);

		if (!(adapter->test_icr & mask)) {
			*data = 4;
			break;
		}

		if (!shared_int) {
			/* Disable the other interrupts to be reported in
			 * the cause register and then force the other
			 * interrupts and see if any get posted.  If
			 * an interrupt was posted to the bus, the
			 * test failed.
			 */
			adapter->test_icr = 0;

			/* Flush any pending interrupts */
			wr32(IGC_ICR, ~0);

			wr32(IGC_IMC, ~mask);
			wr32(IGC_ICS, ~mask);
			wrfl();
			usleep_range(10000, 20000);

			if (adapter->test_icr & mask) {
				*data = 5;
				break;
			}
		}
	}

	/* Disable all the interrupts */
	wr32(IGC_IMC, ~0);
	wrfl();
	usleep_range(10000, 20000);

	/* Unhook test interrupt handler */
	if (adapter->msix_entries)
		free_irq(adapter->msix_entries[0].vector, adapter);
	else
		free_irq(irq, adapter);

	return *data;
}

u64 igc_link_test(struct igc_adapter *adapter, u64 *data)
{
	bool link_up;

	*data = 0;

	/* add delay to give enough time for autonegotioation to finish */
	if (adapter->hw.mac.autoneg)
		ssleep(5);

	link_up = igc_has_link(adapter);
	if (!link_up)
		*data = 1;

	return *data;
}
