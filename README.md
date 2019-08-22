# KnowPulse Searches

This module provides custom search functionality for KnowPulse through use of the [Chado Custom Search API](https://github.com/uofs-pulse-binfo/chado_custom_search).

The following searches are provided:
 - Genetic Marker: `search/markers`
 - Germplasm: `search/germplasm/all`
 - Breeding Crosses: `search/germplasm/crosses`
 - Registered Varieties: `search/germplasm/varieties`
 - RILs: `search/germplasm/RILs`

Many of the searches cache options. To update the option cache run `drush php-eval "kp_searches_cache_options();"`
