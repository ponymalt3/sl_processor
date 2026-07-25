# Timing constraints for de0_nano_top.
# derive_pll_clocks picks up c0/c1's actual multiply/divide/phase from the
# ALTPLL instance itself, so the 0deg/-3ns relationship configured in
# pll.vhd is what gets analyzed here, not a separately-maintained copy of it.

create_clock -name CLOCK_50 -period 20.000 [get_ports CLOCK_50]

derive_pll_clocks
derive_clock_uncertainty
