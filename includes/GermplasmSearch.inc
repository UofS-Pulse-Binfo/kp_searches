<?php
/**
 * Provides a search for all chado stock-based Tripal Content regardless of type.
 *
 */
class GermplasmSearch extends ChadoCustomSearch {

  /**
   * The human readable title of this search. This will be used in listings
   * and shown on the search page as the title.
   */
  public static $title = 'Germplasm Search';

  /**
   * The machine name of the module providing this search.
   */
  public static $module = 'kp_searches';

  /**
   * A description of this search. This is shown at the top of the search page
   * and used for the menu item.
   */
  public static $description = '';

  /**
   * The machine names of the permissions with access to this search. This is
   * used to map your search to existing permissions. It must be an array and
   * is used in hook_menu(). Some examples include 'access content'
   * and 'administer tripal'.
   */
  public static $permissions = ['access content'];

  /**
   * If TRUE, this search will require the submit button to be clicked before
   * executing the query; whereas, if FALSE it will be executed on the
   * first page load without parameters.
   *
   * NOTE: to control the results without parameters check $this->submitted
   * in getQuery() and if FALSE return your pre-submit query.
   */
  public static $require_submit = TRUE;

  /**
   * Add a pager to search results
   * with $num_items_per_page results on a single page.
   * NOTE: Your query has to handle paging.
   */
  public static $pager = TRUE;
  public static $num_items_per_page = 50;

  /**
   * This defined the hook_menu definition for this search. At a minimum the
   * path is required.
   */
  public static $menu = [
    'path' => 'search/germplasm/all',
    // @todo support menu items.
  ];

  /**
   * Add CSS/JS to the form/results page.
   * NOTE: paths supplied should be relative to $module.
   */
  public static $attached = [
    'css' => [
      'css/all_kp_searches.css',
    ],
    'js' => [],
  ];

  /**
   * Information regarding the fields and filters for this search.
   */
  public static $info = [
    // Lists the columns in your results table.
    'fields' => [
      'name' => [
        'title' => 'Name',
        'entity_link' => [
          'chado_table' => 'stock',
          'id_column' => 'stock_id'
        ],
      ],
      'uniquename' => [
        'title' => 'Accession',
        'entity_link' => [
          'chado_table' => 'stock',
          'id_column' => 'stock_id'
        ],
      ],
      'type' => [
        'title' => 'Category',
      ],
      'source_institute' => [
        'title' => 'Source Institute',
      ],
      'species' => [
        'title' => 'Source Species',
        'entity_link' => [
          'chado_table' => 'organism',
          'id_column' => 'organism_id'
        ],
      ],
    ],
    // The filter criteria available to the user.
    // This is used to generate a search form which can be altered.
    'filters' => [
      'genus' => [
        'title' => 'Genus',
        'help' => 'The legume genus the germplasm belongs to (e.g. Lens).',
      ],
      'species' => [
        'title' => 'Species',
        'help' => 'The legume species the germplasm belongs to (e.g. culinaris).',
      ],
      'type' => [
        'title' => 'Category',
        'help' => 'The category this germplasm belongs to (e.g. Breeding Cross).',
      ],
      'source_institute' => [
        'title' => 'Source Institute',
        'help' => 'The institute the germplasm was developed or collected by.'
      ],
      'mom' => [
        'title' => 'Maternal Parent',
        'help' => 'The maternal parent of the germplasm you are interested in.'
      ],
      'dad' => [
        'title' => 'Paternal Parent',
        'help' => 'The paternal parent of the germplasm you are interested in.'
      ],
      'name' => [
        'title' => 'Name',
        'help' => 'The name or accession of the germplasm (e.g. CDC Redberry; partial names are accepted).',
      ],
    ],
  ];

  /**
   * Text that should appear on the button at the bottom of the importer
   * form.
   */
  public static $button_text = 'Search';

  /**
   * Generate the filter form.
   *
   * The base class will generate textfields for each filter defined in $info,
   * set defaults, labels and descriptions, as well as, create the search
   * button.
   *
   * Extend this method to alter the filter form.
   */
  public function form($form, $form_state) {
    $form = parent::form($form, $form_state);

    // Add a crop selector.
    $this->addCropFormElement($form, $form_state);

    // Add a Scientific name selector.
    $this->addSpeciesFormElement($form, $form_state);

    // Change category to a drop-down.
    $options = unserialize(variable_get('kp_searches_germplasm_type_options', 'a:0:{}'));
    $form['type']['#type'] = 'select';
    $form['type']['#options'] = $options;
    $form['type']['#empty_option'] = '- Select -';
    $form['type']['#weight'] = -7;

    // Add a Maternal/paternal name selector.
    $this->addparentageFormElement($form, $form_state);

    // Move name to the bottom.
    $form['name']['#weight'] = 10;

    return $form;
  }

  /**
   * Adds a KnowPulse-specific crop selector to the form.
   */
  public function addCropFormElement(&$form, &$form_state) {

    // Container.
    $form['crops'] = array(
      '#type' => 'markup',
      '#prefix' => '<div class="crop-element form-item"><label>Crop</label>',
      '#suffix' => '</div>',
      '#weight' => -9,
    );

    // Crop options.
    $options = unserialize(variable_get('kp_searches_crop_options', 'a:0:{}'));

    // Determine the current crop, if one is selected.
    $q = drupal_get_query_parameters();
    if (isset($form_state['values'])) {
      $genus_default = $form_state['values']['genus'];
    }
    elseif (is_array($q) AND isset($q['genus'])) {
      $genus_default = $q['genus'];
    }
    else {
      $genus_default = NULL;
    }

    // Now add the form elements for each crop.
    foreach ($options as $crop_key => $crop) {

      // Determine whether this is the active genus or not.
      $class = 'active';
      if ($genus_default !== NULL AND $genus_default != $crop_key) {
        $class = 'inactive';
      }

      // Format the title as a link to the search.
      $search_url = $this::$menu['path'];
      $search_q = $q;
      $search_q['genus'] = $crop['genus'];
      $search_q['species'] = $crop['crop-species'];
      $crop_link = l($crop['title'], $search_url, [
        'query' => $search_q,
        'attributes' => ['class' => [$class]]
      ]);

      // Format the image as a link to the search.
      $image = theme('image', [
        'path' => url($crop['image']),
        'width' => 75,
        'height' => 75,
        'alt' => $crop['title'],
      ]);
      $crop_img_link = l($image, $search_url, [
        'html' => TRUE,
        'query' => $search_q,
        'attributes' => ['class' => [$class]]
      ]);

      // Contain both the image and title links in a container
      // and add to the form.
      $form['crops'][$crop_key] = [
        '#type' => 'markup',
        '#markup' => '<span class="crop">'
           . '<div class="crop-img">' . $crop_img_link . '</div>'
           . '<div class="crop-title">' . $crop_link . '</div>'
           . '</span>',
      ];
    }
  }

  /**
   * Adds a Scientific name field with the genus set based on crop.
   * @todo make this AJAX-driven.
   */
  public function addSpeciesFormElement(&$form, &$form_state) {

    $description = 'The scientific name (genus, species) for the germplasm you are interested in.';
    $form['scientific_name'] = array(
      '#type' => 'markup',
      '#prefix' => '<div id="scientific-nameform-item" class="scientific-name-element form-item"><label>Scientific Name</label>',
      '#suffix' => '<div class="description">' . $description . '</div></div>',
      '#weight' => -8,
    );

    $options = unserialize(variable_get('kp_searches_crop_options', 'a:0:{}'));
    $genus_options = [];
    foreach ($options as $genus => $crop_details) {
      $genus_options[ $genus ] = $genus;
    }

    $q = drupal_get_query_parameters();
    if (isset($form_state['values'])
      AND isset($options[ $form_state['values']['genus'] ])) {
      $species = $options[ $form_state['values']['genus'] ]['species'];
      $genus_default = $form_state['values']['genus'];
      $species_default = $form_state['values']['species'];
    }
    elseif (isset($q['genus']) AND isset($options[ $q['genus'] ])) {
      $species = $options[ $q['genus'] ]['species'];
      $genus_default = $q['genus'];
      $species_default = $q['species'];
    }
    else {
      $genus_default = NULL;
      $species = [];
      $species_default = NULL;
    }

    $form['scientific_name']['genus'] = $form['genus'];
    $form['scientific_name']['genus']['#required'] = TRUE;
    $form['scientific_name']['genus']['#type'] = 'select';
    $form['scientific_name']['genus']['#options'] = $genus_options;
    $form['scientific_name']['genus']['#empty_option'] = '- Select Genus -';
    $form['scientific_name']['genus']['#default_value'] = $genus_default;
    $form['scientific_name']['genus']['#attributes'] = ['onchange' => 'this.form.submit();'];
    unset($form['genus']);


    $form['scientific_name']['species'] = $form['species'];
    $form['scientific_name']['species']['#type'] = 'select';
    $form['scientific_name']['species']['#options'] = $species;
    $form['scientific_name']['species']['#empty_option'] = '- Select Species -';
    $form['scientific_name']['species']['#default_value'] = $species_default;
    unset($form['species']);
  }

  /**
   * Adds a Maternal / Paternal Parent filter element.
   */
  public function addparentageFormElement(&$form, &$form_state) {

    $description = 'The parents of the germplasm you are interested in (single parents welcomed).';
    $form['parentage'] = array(
      '#type' => 'markup',
      '#prefix' => '<div class="parentage-element form-item"><label>Parentage</label>',
      '#suffix' => '<div class="description">' . $description . '</div></div>',
      '#weight' => 9,
    );

    $q = drupal_get_query_parameters();
    if (isset($form_state['values'])) {
      $mom_default = $form_state['values']['mom'];
      $dad_default = $form_state['values']['dad'];
    }
    elseif (isset($q['mom'])) {
      $mom_default = $q['mom'];
      $dad_default = $q['dad'];
    }
    else {
      $mom_default = '';
      $dad_default = '';
    }

    $form['parentage']['mom'] = $form['mom'];
    $form['parentage']['mom']['#attributes']['placeholder'] = 'Maternal Parent';

    $form['parentage']['mom']['#default_value'] = $mom_default;
    unset($form['mom']);

    $form['parentage']['divider'] = [
      '#type' => 'markup',
      '#markup' => '/',
      '#prefix' => '<span class="divider">',
      '#suffix' => '</span>',
    ];

    $form['parentage']['dad'] = $form['dad'];
    $form['parentage']['dad']['#attributes']['placeholder'] = 'Paternal Parent';

    $form['parentage']['dad']['#default_value'] = $dad_default;
    unset($form['dad']);

    // Add autocompletes if genus is available.
    if (isset($form['scientific_name']['genus']['#default_value'])
      AND $form['scientific_name']['genus']['#default_value'] != '') {

      $genus = $form['scientific_name']['genus']['#default_value'];
      $form['parentage']['mom']['#autocomplete_path'] = 'search/germplasm/autocomplete.json/'.$genus.'/all';
      $form['parentage']['dad']['#autocomplete_path'] = 'search/germplasm/autocomplete.json/'.$genus.'/all';
    }
  }

  /**
   * Allows custom searches to validate the form results.
   * Use form_set_error() to signal invalid values.
   */
  public function validateForm($form, $form_state) {

  }

  /**
   * Determine the query for the genetic marker search.
   *
   * @param string $query
   *   The full SQL query to execute. This will be executed using chado_query()
   *   so use curly brackets appropriately. Use :placeholders for any values.
   * @param array $args
   *   An array of arguments to pass to chado_query(). Keys must be the
   *   placeholders in the query and values should be what you want them set to.
   */
  public function getQuery(&$query, &$args, $offset) {
    global $user;

    // Retrieve the filter results already set.
    $filter_results = $this->values;
    // @debug dpm($filter_results, '$filter_results');

    $query = "SELECT
        o.genus||' '||o.species as species,
        o.organism_id,
        s.name, s.uniquename,
        s.stock_id,
        tb.label as type,
        source.value as source_institute
      FROM {stock} s
      LEFT JOIN {organism} o ON o.organism_id=s.organism_id
      LEFT JOIN {stockprop} source ON source.stock_id=s.stock_id AND source.type_id=3711
      LEFT JOIN chado_bundle cb ON cb.type_id=s.type_id
      LEFT JOIN tripal_bundle tb ON tb.id=cb.bundle_id ";

    $where = [];
    $joins = [];

    // -- Genus.
    if ($filter_results['genus'] != '') {
      $where[] = "o.genus ~ :genus";
      $args[':genus'] = $filter_results['genus'];
    }

    // -- Species.
    if ($filter_results['species'] != '') {
      $where[] = "o.species ~ :species";
      $args[':species'] = $filter_results['species'];
    }

    // -- Type.
    if ($filter_results['type'] != '') {
      $where[] = "(s.type_id = :type)";
      $args[':type'] = $filter_results['type'];
    }

    // -- Source Institute.
    if ($filter_results['source_institute'] != '') {
      $where[] = "source.value ~ :source_institute";
      $args[':source_institute'] = $filter_results['source_institute'];
    }

    // -- Name.
    if ($filter_results['name'] != '') {
      $where[] = "(s.name ~ :name OR s.uniquename = :name)";
      $args[':name'] = $filter_results['name'];
    }

    // -- Maternal Parent.
    if ($filter_results['mom'] != '') {
      $joins[] = "LEFT JOIN {stock_relationship} momr ON momr.object_id=s.stock_id
        AND momr.type_id=3632";
      $joins[] = "LEFT JOIN {stock} mom ON momr.subject_id=mom.stock_id";
      $where[] = "(mom.name ~ :mom OR mom.uniquename = :mom)";
      $args[':mom'] = $filter_results['mom'];
    }

    // -- Paternal Parent.
    if ($filter_results['dad'] != '') {
      $joins[] = "LEFT JOIN {stock_relationship} dadr ON dadr.object_id=s.stock_id
        AND dadr.type_id=3633";
      $joins[] = "LEFT JOIN {stock} dad ON dadr.subject_id=dad.stock_id";
      $where[] = "(dad.name ~ :dad OR dad.uniquename = :dad)";
      $args[':dad'] = $filter_results['dad'];
    }

    // -- ADD IN ACCESS CONTROL!
    // Only use access control for non administrators. This is put in place to
    // allow administrators to see content "on the cutting room floor"
    // (i.e. not of any given type).
    $administrator = array_search('administrator', $user->roles);
    if (!$administrator) {
      // See https://github.com/tripal/tripal/blob/7.x-3.x/tripal/includes/tripal.entity.inc#L495
      $entity_types = db_query("SELECT cb.bundle_id FROM chado_bundle cb
        WHERE cb.data_table = 'stock'")->fetchCol();
      // Only includes bundles the current user has access to.
      $available_bundles = [];
      foreach($entity_types as $bundle_id) {
        $bundle_name = 'bio_data_' . $bundle_id;
        if (user_access('view ' . $bundle_name)) {
          $available_bundles[] = $bundle_id;
        }
      }
      $where[] = 'cb.bundle_id IN (:bundle_ids)';
      $args[':bundle_ids'] = $available_bundles;
    }

    // Finally, add it to the query.
    if (!empty($joins)) {
      $query .= implode("\n",$joins);
    }
    if (!empty($where)) {
      $query .= "\n" . ' WHERE ' . implode(' AND ',$where);
    }

    // Sort even though it is SLOW b/c ppl expect it.
    $query .= "\n" . ' ORDER BY s.name ASC';

    // Handle paging.
    $limit = $this::$num_items_per_page + 1;
    $query .= "\n" . ' LIMIT ' . $limit . ' OFFSET ' . $offset;

    // @debug dpm(strtr(str_replace(['{','}'], ['chado.', ''], $query), $args), 'query');
  }
}